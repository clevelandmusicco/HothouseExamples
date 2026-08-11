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

#include <math.h>

using clevelandmusicco::Hothouse;
using clevelandmusicco::HothouseParameter;
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

// Knobs
constexpr Pin PIN_KNOB_1 = daisy::seed::D16;
constexpr Pin PIN_KNOB_2 = daisy::seed::D17;
constexpr Pin PIN_KNOB_3 = daisy::seed::D18;
constexpr Pin PIN_KNOB_4 = daisy::seed::D19;
constexpr Pin PIN_KNOB_5 = daisy::seed::D20;
constexpr Pin PIN_KNOB_6 = daisy::seed::D21;

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

// Standard MIDI button convention: CC >= this value reads as "pressed".
constexpr uint8_t kFootswitchCcOnThreshold = 64;

// CC 0-127 split into thirds, ascending to match TOGGLESWITCH_UP/MIDDLE/DOWN.
static Hothouse::ToggleswitchPosition QuantizeToggleswitchCc(uint8_t value) {
  if (value < 43) return Hothouse::TOGGLESWITCH_UP;
  if (value < 86) return Hothouse::TOGGLESWITCH_MIDDLE;
  return Hothouse::TOGGLESWITCH_DOWN;
}

const uint32_t Hothouse::HOLD_THRESHOLD_MS;

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

    if (knob_cc_active_[i]) {
      // Baseline is frozen at hand-off (see the else branch below), so a
      // slow turn still accumulates real distance: a normal turn moves
      // the pot well under 1% of range within a single ~1ms audio block.
      float delta = knobs[i].Value() - knob_cc_last_raw_[i];
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
  MidiUsbHandler::Config midi_cfg;
  midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
  midi_.Init(midi_cfg);
}

void Hothouse::ProcessMidi() {
  midi_.Listen();
  while (midi_.HasEvents()) {
    auto msg = midi_.PopEvent();
    switch (msg.type) {
      case daisy::ControlChange: {
        auto cc = msg.AsControlChange();
        for (size_t i = 0; i < KNOB_LAST; i++) {
          if (cc.control_number == kKnobCcNumber[i]) {
            knob_cc_value_[i] = cc.value / 127.0f;
            knob_cc_active_[i] = true;
          }
        }
        for (size_t i = 0; i < TOGGLESWITCH_LAST; i++) {
          if (cc.control_number == kToggleswitchCcNumber[i]) {
            toggle_cc_value_[i] = QuantizeToggleswitchCc(cc.value);
            toggle_cc_active_[i] = true;
          }
        }
        for (size_t i = 0; i < 2; i++) {
          if (cc.control_number == kFootswitchCcNumber[i]) {
            footswitch_cc_pressed_[i] = cc.value >= kFootswitchCcOnThreshold;
          }
        }
      } break;
      case daisy::ProgramChange: {
        program_number_ = msg.AsProgramChange().program;
      } break;
      default:
        if (midi_event_callback_ != NULL) {
          midi_event_callback_(msg);
        }
        break;
    }
  }
}

void Hothouse::RegisterMidiEventCallback(MidiEventCallback callback) {
  midi_event_callback_ = callback;
}

int16_t Hothouse::GetProgramNumber() { return program_number_; }

void Hothouse::ProcessDigitalControls() {
  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Debounce();
  }
  ProcessFootswitchPresses(FOOTSWITCH_1);
  ProcessFootswitchPresses(FOOTSWITCH_2);
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

// Public convenience function to get position of toggleswitches 1-3.
Hothouse::ToggleswitchPosition Hothouse::GetToggleswitchPosition(
    Toggleswitch tsw) {
  ToggleswitchPosition physical;
  switch (tsw) {
    case (TOGGLESWITCH_1):
      physical = GetLogicalSwitchPosition(switches[SWITCH_1_UP],
                                          switches[SWITCH_1_DOWN]);
      break;
    case (TOGGLESWITCH_2):
      physical = GetLogicalSwitchPosition(switches[SWITCH_2_UP],
                                          switches[SWITCH_2_DOWN]);
      break;
    case (TOGGLESWITCH_3):
      physical = GetLogicalSwitchPosition(switches[SWITCH_3_UP],
                                          switches[SWITCH_3_DOWN]);
      break;
    default:
      seed.PrintLine(
          "ERROR: Unexpected value provided for Toggleswitch 'tsw'. "
          "Returning TOGGLESWITCH_UNKNOWN by default.");
      return TOGGLESWITCH_UNKNOWN;
  }

  // Discrete state doesn't drift block-to-block like a knob's raw ADC
  // value, so (unlike GetKnobValue) comparing directly here is enough.
  if (toggle_cc_active_[tsw] && physical != toggle_last_physical_[tsw]) {
    toggle_cc_active_[tsw] = false;
  }
  toggle_last_physical_[tsw] = physical;

  return toggle_cc_active_[tsw] ? toggle_cc_value_[tsw] : physical;
}

bool Hothouse::GetFootswitchPressed(Switches footswitch) {
  int idx = footswitch == FOOTSWITCH_1 ? 0 : 1;
  return switches[footswitch].Pressed() || footswitch_cc_pressed_[idx];
}

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

Hothouse::ToggleswitchPosition Hothouse::GetLogicalSwitchPosition(Switch up,
                                                                  Switch down) {
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
  bool is_pressed = switches[footswitch].RisingEdge();
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

  if (switches[footswitch].Pressed() && press_duration >= HOLD_THRESHOLD_MS && !footswitch_long_press_triggered[footswitch_index]) {
    // Both footswitches held = DFU gesture; don't fire a long-press callback
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

void HothouseParameter::Init(Hothouse &hw, Hothouse::Knob knob, float min,
                              float max, Parameter::Curve curve) {
  hw_ = &hw;
  knob_ = knob;
  pmin_ = min;
  pmax_ = max;
  curve_ = curve;
  lmin_ = logf(min < 0.0000001f ? 0.0000001f : min);
  lmax_ = logf(max);
}

float HothouseParameter::Process() {
  float in = hw_->GetKnobValue(knob_);
  switch (curve_) {
    case Parameter::LINEAR:
      val_ = (in * (pmax_ - pmin_)) + pmin_;
      break;
    case Parameter::EXPONENTIAL:
      val_ = ((in * in) * (pmax_ - pmin_)) + pmin_;
      break;
    case Parameter::LOGARITHMIC:
      val_ = expf((in * (lmax_ - lmin_)) + lmin_);
      break;
    case Parameter::CUBE:
      val_ = ((in * (in * in)) * (pmax_ - pmin_)) + pmin_;
      break;
    default:
      break;
  }
  return val_;
}