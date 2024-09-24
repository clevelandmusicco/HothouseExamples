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

#include "optional"

using clevelandmusicco::Hothouse;
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
  }
}

float Hothouse::GetKnobValue(Knob k) {
  size_t idx;
  idx = k < KNOB_LAST ? k : KNOB_1;
  return knobs[idx].Value();
}

void Hothouse::ProcessDigitalControls() {
  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Debounce();
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

// Public convenience function to get position of toggleswitches 1-3.
Hothouse::ToggleswitchPosition Hothouse::GetToggleswitchPosition(
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
      seed.PrintLine(
          "ERROR: Unexpected value provided for Toggleswitch 'tsw'. "
          "Returning TOGGLESWITCH_UNKNOWN by default.");
      return TOGGLESWITCH_UNKNOWN;
  }
}

void Hothouse::CheckResetToBootloader() {
  if (switches[Hothouse::FOOTSWITCH_1].Pressed()) {
    if (footswitch1_start_time == 0) {
      footswitch1_start_time = System::GetNow();
    } else if (System::GetNow() - footswitch1_start_time >= HOLD_THRESHOLD_MS) {
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

      // Reset system to bootloader after LED flashing
      System::ResetToBootloader();
    }
  } else {
    // Reset the hold timer if the footswitch is released
    footswitch1_start_time = 0;
  }
}

Hothouse::ToggleswitchPosition Hothouse::GetLogicalSwitchPosition(Switch up,
                                                                  Switch down) {
  return up.Pressed()
             ? TOGGLESWITCH_UP
             : (down.Pressed() ? TOGGLESWITCH_DOWN : TOGGLESWITCH_MIDDLE);
}
