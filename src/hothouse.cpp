// Hardware proxy for Hothouse DIY DSP Platform
// Copyright (C) 2024  Cleveland Music Co.  <code@clevelandmusicco.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "hothouse.h"

#include <cmath>

using clevelandmusicco::Hothouse;
using clevelandmusicco::HothouseParameter;
using daisy::GPIO;
using daisy::MidiUsbTransport;
using daisy::System;

#ifndef SAMPLE_RATE
// #define SAMPLE_RATE DSY_AUDIO_SAMPLE_RATE
#define SAMPLE_RATE 48014.f
#endif

// Hardware related defines.
// Switches
constexpr Pin PIN_SW_1_UP = daisy::seed::D9;
constexpr Pin PIN_SW_1_DOWN = daisy::seed::D10;
constexpr Pin PIN_SW_2_UP = daisy::seed::D7;
constexpr Pin PIN_SW_2_DOWN = daisy::seed::D8;
constexpr Pin PIN_SW_3_UP = daisy::seed::D5;
constexpr Pin PIN_SW_3_DOWN = daisy::seed::D6;
constexpr Pin PIN_FSW_1 = daisy::seed::D25;
constexpr Pin PIN_FSW_2 = daisy::seed::D26;

// Footswitch LEDs. Same pins as the Hothouse::LED_1/LED_2 enum, but as a
// daisy::Pin: seed.GetPin() still hands back the legacy dsy_gpio_pin.
constexpr Pin PIN_LED_1 = daisy::seed::D22;
constexpr Pin PIN_LED_2 = daisy::seed::D23;

// Knobs
constexpr Pin PIN_KNOB_1 = daisy::seed::D16;
constexpr Pin PIN_KNOB_2 = daisy::seed::D17;
constexpr Pin PIN_KNOB_3 = daisy::seed::D18;
constexpr Pin PIN_KNOB_4 = daisy::seed::D19;
constexpr Pin PIN_KNOB_5 = daisy::seed::D20;
constexpr Pin PIN_KNOB_6 = daisy::seed::D21;

namespace {

// CC map for the 6 knobs, one per index. 14-19 sit in MIDI's undefined
// controller range (no collision with mod wheel/volume/pan/etc). File
// scope, not a class member: array statics ODR-fail on this toolchain.
constexpr uint8_t kKnobCcNumber[Hothouse::KNOB_LAST] = {14, 15, 16, 17, 18, 19};

// Raw knob delta (0-1 range) that counts as the physical pot being "touched"
// and reclaiming control from a MIDI CC override.
constexpr float kKnobCcTouchThreshold = 0.01f;

// CC map continues contiguously: one per toggleswitch, then one per
// footswitch. Same undefined range as the knobs.
constexpr uint8_t kToggleswitchCcNumber[Hothouse::TOGGLESWITCH_LAST] = {20, 21,
                                                                        22};
constexpr uint8_t kFootswitchCcNumber[2] = {23, 24};

// Whole-pedal bypass. Its own CC rather than a footswitch one, because a host
// wants to set bypass outright; the footswitch CCs model momentary presses.
constexpr uint8_t kBypassCcNumber = 25;

// Standard MIDI button convention: CC >= this value reads as "pressed"
// (and, for kBypassCcNumber, as "engaged").
constexpr uint8_t kFootswitchCcOnThreshold = 64;

// CC 0-127 split into thirds, ascending to match TOGGLESWITCH_UP/MIDDLE/DOWN.
constexpr uint8_t kToggleswitchCcMiddle = 43;
constexpr uint8_t kToggleswitchCcDown = 86;

// Floor applied before log(min) so a 0.0 range bottom stays finite. Same
// value daisy::Parameter uses.
constexpr float kLogCurveMinInput = 0.0000001f;

// Settings block layout version. A block that doesn't match is thrown away and
// the compile-time defaults are used, so old QSPI contents can't be misread.
constexpr uint16_t kSettingsVersion = 1;

constexpr uint8_t kMidiChannelMax = 16;

// Clamped here rather than at the use site so a bogus -DHOTHOUSE_MIDI_CHANNEL
// falls back to omni instead of matching nothing.
constexpr uint8_t kCompileTimeMidiChannel =
    (HOTHOUSE_MIDI_CHANNEL) <= kMidiChannelMax ? (HOTHOUSE_MIDI_CHANNEL) : 0;

// Boot gesture timings, all in ms.
constexpr uint32_t kSwitchSettleMs = 20;      // enough Debounce() shifts to latch
constexpr uint32_t kLearnTimeoutMs = 10000;   // give up and keep the old channel
constexpr uint32_t kLearnBlinkMs = 200;       // "listening" flash, both LEDs
constexpr uint32_t kBlinkOnMs = 100;          // channel readout digit
constexpr uint32_t kBlinkOffMs = 150;
constexpr uint32_t kBlinkDigitGapMs = 300;
constexpr uint32_t kLongFlashMs = 600;        // "nothing changed"

Hothouse::ToggleswitchPosition QuantizeToggleswitchCc(uint8_t value) {
  if (value < kToggleswitchCcMiddle) return Hothouse::TOGGLESWITCH_UP;
  if (value < kToggleswitchCcDown) return Hothouse::TOGGLESWITCH_MIDDLE;
  return Hothouse::TOGGLESWITCH_DOWN;
}

// True for message types that carry a channel nibble. System Common and System
// Real Time (clock, sysex, transport) don't, so they're never channel-filtered.
bool IsChannelMessage(const daisy::MidiEvent& msg) {
  switch (msg.type) {
    case daisy::NoteOff:
    case daisy::NoteOn:
    case daisy::PolyphonicKeyPressure:
    case daisy::ControlChange:
    case daisy::ProgramChange:
    case daisy::ChannelPressure:
    case daisy::PitchBend:
    case daisy::ChannelMode:
      return true;
    default:
      return false;
  }
}

// Channel learn ignores ChannelMode: libDaisy parses CC 120-127 into it, and a
// host that broadcasts "reset all controllers" at startup would otherwise pick
// the channel for you.
bool IsChannelLearnMessage(const daisy::MidiEvent& msg) {
  return IsChannelMessage(msg) && msg.type != daisy::ChannelMode;
}

// Both footswitch LEDs as plain on/off GPIO. daisy::Led is software PWM and
// wants a steady Update() cadence; boot-time blinking has no use for that.
class BootLeds {
 public:
  void Init(Pin led_1, Pin led_2) {
    leds_[0].Init(led_1, GPIO::Mode::OUTPUT);
    leds_[1].Init(led_2, GPIO::Mode::OUTPUT);
    Set(false, false);
  }

  void Set(bool led_1_on, bool led_2_on) {
    leds_[0].Write(led_1_on);
    leds_[1].Write(led_2_on);
  }

 private:
  GPIO leds_[2];
};

// One decimal digit as `count` flashes on a single LED.
void BlinkDigit(BootLeds* leds, size_t led_index, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    leds->Set(led_index == 0, led_index == 1);
    System::Delay(kBlinkOnMs);
    leds->Set(false, false);
    System::Delay(kBlinkOffMs);
  }
}

}  // namespace

const uint32_t Hothouse::HOLD_THRESHOLD_MS;
const uint8_t Hothouse::MIDI_CHANNEL_OMNI;

void Hothouse::Init(bool boost) {
  // Initialize the hardware.
  seed.Configure();
  seed.Init(boost);
  InitSwitches();
  InitAnalogControls();
  SetAudioBlockSize(48);
}

void Hothouse::DelayMs(size_t del) { seed.DelayMs(del); }

void Hothouse::SetHidUpdateRates() {
  for (size_t i = 0; i < KNOB_LAST; i++) {
    knobs[i].SetSampleRate(AudioCallbackRate());
  }
}

void Hothouse::StartAudio(AudioHandle::InterleavingAudioCallback cb) {
  seed.StartAudio(cb);
}

void Hothouse::StartAudio(AudioHandle::AudioCallback cb) {
  seed.StartAudio(cb);
}

void Hothouse::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb) {
  seed.ChangeAudioCallback(cb);
}

void Hothouse::ChangeAudioCallback(AudioHandle::AudioCallback cb) {
  seed.ChangeAudioCallback(cb);
}

void Hothouse::StopAudio() { seed.StopAudio(); }

void Hothouse::SetAudioBlockSize(size_t size) {
  seed.SetAudioBlockSize(size);
  SetHidUpdateRates();
}

size_t Hothouse::AudioBlockSize() { return seed.AudioBlockSize(); }

void Hothouse::SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate) {
  seed.SetAudioSampleRate(samplerate);
  SetHidUpdateRates();
}

float Hothouse::AudioSampleRate() { return seed.AudioSampleRate(); }

float Hothouse::AudioCallbackRate() { return seed.AudioCallbackRate(); }

void Hothouse::StartAdc() { seed.adc.Start(); }

void Hothouse::StopAdc() { seed.adc.Stop(); }

void Hothouse::ProcessAnalogControls() {
  for (size_t i = 0; i < KNOB_LAST; i++) {
    knobs[i].Process();

    const uint8_t seq = knob_cc_seq_[i];
    if (seq != knob_cc_seen_[i]) {
      // Hand-off freezes the baseline, so a slow turn accumulates real
      // distance against a fixed point: a normal turn moves the pot well
      // under 1% of range within a single ~1ms audio block.
      knob_cc_seen_[i] = seq;
      knob_cc_active_[i] = true;
      knob_cc_last_raw_[i] = knobs[i].Value();
    } else if (knob_cc_active_[i]) {
      const float delta = knobs[i].Value() - knob_cc_last_raw_[i];
      if (delta > kKnobCcTouchThreshold || delta < -kKnobCcTouchThreshold) {
        // Physical knob moved, so it reclaims control from the CC override.
        knob_cc_active_[i] = false;
        knob_cc_last_raw_[i] = knobs[i].Value();
      }
    } else {
      knob_cc_last_raw_[i] = knobs[i].Value();
    }
  }
}

float Hothouse::GetKnobValue(Knob k) {
  size_t idx;
  idx = k < KNOB_LAST ? k : KNOB_1;
  if (knob_cc_active_[idx]) {
    return knob_cc_value_[idx];
  }
  return knobs[idx].Value();
}

void Hothouse::StartMidi() {
  LoadSettings();
  MidiUsbHandler::Config midi_cfg;
  midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
  midi_.Init(midi_cfg);
  RunMidiChannelGestures();
}

uint8_t Hothouse::GetMidiChannel() const { return midi_channel_; }

void Hothouse::SetMidiChannel(uint8_t channel) {
  if (channel > kMidiChannelMax) return;
  LoadSettings();
  midi_channel_ = channel;
  Settings& settings = settings_storage_.GetSettings();
  settings.version = kSettingsVersion;
  settings.midi_channel = channel;
  settings.reserved = 0;
  settings_storage_.Save();
}

// Idempotent: SetMidiChannel() can be called before StartMidi(), and saving
// through an uninitialized storage object would write to QSPI offset 0.
void Hothouse::LoadSettings() {
  if (settings_loaded_) return;
  settings_loaded_ = true;

  Settings defaults;
  defaults.version = kSettingsVersion;
  defaults.midi_channel = kCompileTimeMidiChannel;
  defaults.reserved = 0;
  settings_storage_.Init(defaults, HOTHOUSE_SETTINGS_QSPI_OFFSET);

  const Settings& saved = settings_storage_.GetSettings();
  if (saved.version == kSettingsVersion &&
      saved.midi_channel <= kMidiChannelMax) {
    midi_channel_ = saved.midi_channel;
  } else {
    // Older layout or garbage. Lay down this build's defaults rather than
    // reinterpreting bytes that meant something else.
    settings_storage_.RestoreDefaults();
    midi_channel_ = defaults.midi_channel;
  }
}

// Boot-time channel config, identical on every effect that calls StartMidi()
// so the gesture is worth learning once. Runs before StartAudio(), which is
// what makes the blocking QSPI writes and LED delays safe here.
void Hothouse::RunMidiChannelGestures() {
  // Nothing has called ProcessAllControls() yet, so the debouncers still read
  // "released"; give them enough shifts to latch the real state.
  DebounceFootswitches(kSwitchSettleMs);
  const bool fsw_1 = switches[FOOTSWITCH_1].Pressed();
  const bool fsw_2 = switches[FOOTSWITCH_2].Pressed();

  // Each gesture wants its footswitch alone: both held is the DFU grip, and
  // that shouldn't reconfigure MIDI on the way to the bootloader.
  if (fsw_1 && !fsw_2) {
    if (RunMidiChannelLearn()) BlinkChannel(midi_channel_);
    return;
  }
  if (fsw_2 && !fsw_1) {
    SetMidiChannel(MIDI_CHANNEL_OMNI);
    BlinkChannel(MIDI_CHANNEL_OMNI);
    return;
  }

  // Omni boots silently, so users who never touch MIDI see no change.
  if (midi_channel_ != MIDI_CHANNEL_OMNI) BlinkChannel(midi_channel_);
}

// \return true if a channel was learned, false on timeout.
bool Hothouse::RunMidiChannelLearn() {
  BootLeds leds;
  leds.Init(PIN_LED_1, PIN_LED_2);

  const uint32_t start = System::GetNow();
  uint32_t last_blink = start;
  bool blink_on = true;
  leds.Set(true, true);

  while (System::GetNow() - start < kLearnTimeoutMs) {
    const uint32_t now = System::GetNow();
    if (now - last_blink >= kLearnBlinkMs) {
      last_blink = now;
      blink_on = !blink_on;
      leds.Set(blink_on, blink_on);
    }

    midi_.Listen();
    while (midi_.HasEvents()) {
      auto msg = midi_.PopEvent();
      if (!IsChannelLearnMessage(msg)) continue;
      leds.Set(false, false);
      // Wire channels are 0-15; this class talks 1-16 with 0 meaning omni.
      SetMidiChannel(static_cast<uint8_t>(msg.channel + 1));
      return true;
    }
  }

  // Timed out. One long flash on both LEDs means "nothing changed".
  leds.Set(true, true);
  System::Delay(kLongFlashMs);
  leds.Set(false, false);
  return false;
}

void Hothouse::DebounceFootswitches(uint32_t duration_ms) {
  const uint32_t start = System::GetNow();
  while (System::GetNow() - start < duration_ms) {
    switches[FOOTSWITCH_1].Debounce();
    switches[FOOTSWITCH_2].Debounce();
    System::Delay(1);
  }
}

// Two LEDs as a two-digit readout: LED_1 flashes the tens, LED_2 the ones.
// Channel 1 is one flash on LED_2 alone; channel 10 is one flash on LED_1
// alone. Omni has no digits, so both LEDs blink together twice instead.
void Hothouse::BlinkChannel(uint8_t channel) {
  BootLeds leds;
  leds.Init(PIN_LED_1, PIN_LED_2);

  if (channel == MIDI_CHANNEL_OMNI) {
    for (int i = 0; i < 2; i++) {
      leds.Set(true, true);
      System::Delay(kBlinkOnMs * 2);
      leds.Set(false, false);
      System::Delay(kBlinkOffMs * 2);
    }
    return;
  }

  BlinkDigit(&leds, 0, channel / 10);
  System::Delay(kBlinkDigitGapMs);
  BlinkDigit(&leds, 1, channel % 10);
}

void Hothouse::ProcessMidi() {
  midi_.Listen();
  while (midi_.HasEvents()) {
    auto msg = midi_.PopEvent();

    // Wrong channel: dropped outright, so the effect's own CCs obey the same
    // channel as the built-in map. Clock and sysex carry no channel and pass.
    if (midi_channel_ != MIDI_CHANNEL_OMNI && IsChannelMessage(msg) &&
        msg.channel != midi_channel_ - 1) {
      continue;
    }

    bool consumed = false;
    switch (msg.type) {
      case daisy::ControlChange:
        consumed = HandleControlChange(msg.AsControlChange());
        break;
      case daisy::ProgramChange:
        // Latched for GetProgramNumber(), but still forwarded so an effect
        // can act on the change instead of polling for it.
        program_number_ = msg.AsProgramChange().program;
        break;
      default:
        break;
    }
    if (!consumed && midi_event_callback_ != nullptr) {
      midi_event_callback_(msg);
    }
  }
}

// Deposits the CC into the shared state the audio ISR picks up. Value is
// written before the sequence number so the ISR can never observe a bump
// without the data behind it.
bool Hothouse::HandleControlChange(const daisy::ControlChangeEvent& cc) {
  for (size_t i = 0; i < KNOB_LAST; i++) {
    if (cc.control_number == kKnobCcNumber[i]) {
      knob_cc_value_[i] = cc.value / 127.0f;
      knob_cc_seq_[i] = knob_cc_seq_[i] + 1;
      return true;
    }
  }
  for (size_t i = 0; i < TOGGLESWITCH_LAST; i++) {
    if (cc.control_number == kToggleswitchCcNumber[i]) {
      toggle_cc_value_[i] = QuantizeToggleswitchCc(cc.value);
      toggle_cc_seq_[i] = toggle_cc_seq_[i] + 1;
      return true;
    }
  }
  for (size_t i = 0; i < 2; i++) {
    if (cc.control_number == kFootswitchCcNumber[i]) {
      const bool pressed = cc.value >= kFootswitchCcOnThreshold;
      // Edge is counted here, not in the ISR: a 127-then-0 pair can arrive
      // within one audio block and the ISR would only ever see the 0.
      if (pressed && !footswitch_cc_pressed_[i]) {
        footswitch_cc_edge_seq_[i] = footswitch_cc_edge_seq_[i] + 1;
      }
      footswitch_cc_pressed_[i] = pressed;
      return true;
    }
  }
  if (cc.control_number == kBypassCcNumber) {
    // Absolute, unlike the footswitch CCs: the host says what the state IS,
    // so a repeated identical value is a no-op rather than another toggle.
    bypass_cc_bypassed_ = cc.value < kFootswitchCcOnThreshold;
    bypass_cc_seq_ = bypass_cc_seq_ + 1;
    return true;
  }
  return false;
}

void Hothouse::RegisterMidiEventCallback(MidiEventCallback callback) {
  midi_event_callback_ = callback;
}

int16_t Hothouse::GetProgramNumber() { return program_number_; }

void Hothouse::ProcessDigitalControls() {
  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Debounce();
  }
  ProcessDigitalCcOverrides();
  ProcessFootswitchPresses(FOOTSWITCH_1);
  ProcessFootswitchPresses(FOOTSWITCH_2);
}

// CC hand-off and reclaim for the discrete controls. Lives here rather than
// in the accessors so the result doesn't depend on how often (or whether) an
// effect happens to call them.
void Hothouse::ProcessDigitalCcOverrides() {
  for (size_t i = 0; i < TOGGLESWITCH_LAST; i++) {
    const ToggleswitchPosition physical =
        ReadPhysicalToggleswitchPosition(static_cast<Toggleswitch>(i));
    const uint8_t seq = toggle_cc_seq_[i];
    if (seq != toggle_cc_seen_[i]) {
      toggle_cc_seen_[i] = seq;
      toggle_cc_active_[i] = true;
    } else if (toggle_cc_active_[i] && physical != toggle_last_physical_[i]) {
      // Discrete state doesn't drift block-to-block like a knob's raw ADC
      // value, so any change from the latched position is a real flip.
      toggle_cc_active_[i] = false;
    }
    // Baseline tracks the switch until CC takes over, then stays frozen.
    if (!toggle_cc_active_[i]) {
      toggle_last_physical_[i] = physical;
    }
  }

  for (size_t i = 0; i < 2; i++) {
    const uint8_t seq = footswitch_cc_edge_seq_[i];
    footswitch_cc_rising_edge_[i] = seq != footswitch_cc_edge_seen_[i];
    footswitch_cc_edge_seen_[i] = seq;
  }

  // Adopting the CC here (rather than in the accessor) keeps bypassed_ to a
  // single writer, and lets a footswitch toggle later in the same audio block
  // override a CC that landed just before it.
  const uint8_t bypass_seq = bypass_cc_seq_;
  if (bypass_seq != bypass_cc_seen_) {
    bypass_cc_seen_ = bypass_seq;
    bypassed_ = bypass_cc_bypassed_;
  }
}

void Hothouse::InitSwitches() {
  constexpr Pin pin_numbers[SWITCH_LAST] = {
      PIN_SW_1_UP, PIN_SW_1_DOWN, PIN_SW_2_UP, PIN_SW_2_DOWN,
      PIN_SW_3_UP, PIN_SW_3_DOWN, PIN_FSW_1,   PIN_FSW_2,
  };

  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Init(pin_numbers[i]);
  }
}

void Hothouse::InitAnalogControls() {
  constexpr Pin knob_pins[KNOB_LAST] = {PIN_KNOB_1, PIN_KNOB_2, PIN_KNOB_3,
                                        PIN_KNOB_4, PIN_KNOB_5, PIN_KNOB_6};

  // Set order of ADCs based on CHANNEL NUMBER
  AdcChannelConfig cfg[KNOB_LAST];

  // Initialize ADC configuration with Single Pins
  for (size_t i = 0; i < KNOB_LAST; ++i) {
    cfg[i].InitSingle(knob_pins[i]);
  }

  // Initialize ADC with configuration
  seed.adc.Init(cfg, KNOB_LAST);

  // Get the audio callback rate once
  float callback_rate = AudioCallbackRate();

  // Initialize knobs with ADC pointers and callback rate
  for (size_t i = 0; i < KNOB_LAST; ++i) {
    knobs[i].Init(seed.adc.GetPtr(i), callback_rate);
  }
}

Hothouse::ToggleswitchPosition Hothouse::ReadPhysicalToggleswitchPosition(
    Toggleswitch tsw) {
  switch (tsw) {
    case (TOGGLESWITCH_1):
      return GetLogicalSwitchPosition(switches[SWITCH_1_UP],
                                      switches[SWITCH_1_DOWN]);
    case (TOGGLESWITCH_2):
      return GetLogicalSwitchPosition(switches[SWITCH_2_UP],
                                      switches[SWITCH_2_DOWN]);
    case (TOGGLESWITCH_3):
      return GetLogicalSwitchPosition(switches[SWITCH_3_UP],
                                      switches[SWITCH_3_DOWN]);
    default:
      return TOGGLESWITCH_UNKNOWN;
  }
}

// Public convenience function to get position of toggleswitches 1-3.
Hothouse::ToggleswitchPosition Hothouse::GetToggleswitchPosition(
    Toggleswitch tsw) {
  const ToggleswitchPosition physical = ReadPhysicalToggleswitchPosition(tsw);
  if (physical == TOGGLESWITCH_UNKNOWN) {
    seed.PrintLine(
        "ERROR: Unexpected value provided for Toggleswitch 'tsw'. "
        "Returning TOGGLESWITCH_UNKNOWN by default.");
    return TOGGLESWITCH_UNKNOWN;
  }
  return toggle_cc_active_[tsw] ? toggle_cc_value_[tsw] : physical;
}

// Collapses FOOTSWITCH_1/2 onto 0/1, rejecting every other Switches value.
bool Hothouse::FootswitchIndex(Switches footswitch, size_t* idx) {
  if (footswitch != FOOTSWITCH_1 && footswitch != FOOTSWITCH_2) {
    seed.PrintLine(
        "ERROR: Unexpected value provided for 'footswitch'. "
        "Returning false by default.");
    return false;
  }
  *idx = footswitch - FOOTSWITCH_1;
  return true;
}

bool Hothouse::GetFootswitchPressed(Switches footswitch) {
  size_t idx;
  if (!FootswitchIndex(footswitch, &idx)) {
    return false;
  }
  return switches[footswitch].Pressed() || footswitch_cc_pressed_[idx];
}

bool Hothouse::GetFootswitchRisingEdge(Switches footswitch) {
  size_t idx;
  if (!FootswitchIndex(footswitch, &idx)) {
    return false;
  }
  return switches[footswitch].RisingEdge() || footswitch_cc_rising_edge_[idx];
}

bool Hothouse::GetBypass() { return bypassed_; }

void Hothouse::SetBypass(bool bypassed) { bypassed_ = bypassed; }

void Hothouse::ToggleBypass() { bypassed_ = !bypassed_; }

void Hothouse::CheckResetToBootloader() {
  if (switches[FOOTSWITCH_1].Pressed() && switches[FOOTSWITCH_2].Pressed()) {
    if (dfu_start_time_ == 0) {
      dfu_start_time_ = System::GetNow();
    } else if (System::GetNow() - dfu_start_time_ >= HOLD_THRESHOLD_MS) {
      // Shut 'er down so the LEDs always flash
      StopAdc();
      StopAudio();

      daisy::Led _led_1, _led_2;
      _led_1.Init(seed.GetPin(22), false);
      _led_2.Init(seed.GetPin(23), false);

      // Alternately flash the LEDs 3 times
      for (int i = 0; i < 3; i++) {
        _led_1.Set(1);
        _led_2.Set(0);
        _led_1.Update();
        _led_2.Update();
        System::Delay(100);

        _led_1.Set(0);
        _led_2.Set(1);
        _led_1.Update();
        _led_2.Update();
        System::Delay(100);
      }

      System::ResetToBootloader();
    }
  } else {
    // Reset the hold timer if either footswitch is released
    dfu_start_time_ = 0;
  }
}

Hothouse::ToggleswitchPosition Hothouse::GetLogicalSwitchPosition(
    const Switch& up, const Switch& down) {
  return up.Pressed()
             ? TOGGLESWITCH_UP
             : (down.Pressed() ? TOGGLESWITCH_DOWN : TOGGLESWITCH_MIDDLE);
}

void Hothouse::RegisterFootswitchCallbacks(FootswitchCallbacks *callbacks) {
  footswitchCallbacks = callbacks;
}

// Watches for normal, double, and long presses of the footswitches.
void Hothouse::ProcessFootswitchPresses(Switches footswitch) {
  if (footswitchCallbacks == NULL) {
    return; // Nothing to do if callbacks have not been registered
  }
  // Read through the accessors, not switches[], so a MIDI CC press drives the
  // same normal/double/long state machine a stomp does.
  bool is_pressed = GetFootswitchRisingEdge(footswitch);
  int footswitch_index = footswitch == Hothouse::FOOTSWITCH_1 ? 0 : 1;

  uint32_t now = System::GetNow();

  if (is_pressed == true && footswitch_last_state[footswitch_index] == false) {
    // Footswitch is pressed
    footswitch_start_time[footswitch_index] = now;

    if ((now - footswitch_last_press_time[footswitch_index]) <= DOUBLE_PRESS_THRESHOLD_MS) {
      footswitch_press_count[footswitch_index]++;
    } else {
      footswitch_press_count[footswitch_index] = 1;
    }

    footswitch_last_press_time[footswitch_index] = now;
    footswitch_long_press_triggered[footswitch_index] = false; // Reset long press trigger when pressed
  }

  uint32_t press_duration = now - footswitch_start_time[footswitch_index];

  if (GetFootswitchPressed(footswitch) && press_duration >= HOLD_THRESHOLD_MS && !footswitch_long_press_triggered[footswitch_index]) {
    // Both footswitches held = DFU gesture; don't fire a long-press callback.
    // Physical-only on purpose: MIDI must not be able to reach DFU.
    bool dfu_gesture = switches[FOOTSWITCH_1].Pressed() && switches[FOOTSWITCH_2].Pressed();
    if (!dfu_gesture && footswitchCallbacks->HandleLongPress != NULL) {
      footswitchCallbacks->HandleLongPress(footswitch);
    }
    footswitch_long_press_triggered[footswitch_index] = true;
  }

  if (is_pressed == false && footswitch_last_state[footswitch_index] == true) {
    // Button released
    if (!footswitch_long_press_triggered[footswitch_index]) {
      if (footswitch_press_count[footswitch_index] >= 2) {
        if (footswitchCallbacks->HandleDoublePress != NULL) {
          footswitchCallbacks->HandleDoublePress(footswitch);
        }
        footswitch_press_count[footswitch_index] = 0;
      } else if (press_duration < HOLD_THRESHOLD_MS && footswitchCallbacks->HandleNormalPress != NULL) {
        footswitchCallbacks->HandleNormalPress(footswitch);
      }
    }
  }

  footswitch_last_state[footswitch_index] = is_pressed;
}

// --- HothouseParameter ---

void HothouseParameter::Init(Hothouse* hw, Hothouse::Knob knob, float min,
                             float max, Parameter::Curve curve) {
  hw_ = hw;
  knob_ = knob;
  pmin_ = min;
  pmax_ = max;
  curve_ = curve;
  lmin_ = std::log(min < kLogCurveMinInput ? kLogCurveMinInput : min);
  lmax_ = std::log(max);
}

float HothouseParameter::Process() {
  if (hw_ == nullptr) {
    return val_;
  }
  const float in = hw_->GetKnobValue(knob_);
  switch (curve_) {
    case Parameter::EXPONENTIAL:
      val_ = ((in * in) * (pmax_ - pmin_)) + pmin_;
      break;
    case Parameter::LOGARITHMIC:
      val_ = std::exp((in * (lmax_ - lmin_)) + lmin_);
      break;
    case Parameter::CUBE:
      val_ = ((in * (in * in)) * (pmax_ - pmin_)) + pmin_;
      break;
    case Parameter::LINEAR:
    case Parameter::LAST:
    default:
      val_ = (in * (pmax_ - pmin_)) + pmin_;
      break;
  }
  return val_;
}
