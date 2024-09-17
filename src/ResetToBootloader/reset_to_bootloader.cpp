// ResetToBootloader for Hothouse DIY DSP Platform
// Copyright (C) 2024 Cleveland Music Co. <code@clevelandmusicco.com>
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

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisy::System;

Hothouse hw;
Led led_bypass, led_1;
bool bypass = true;

// Timer for detecting long press on FOOTSWITCH_1
uint32_t footswitch1_start_time = 0;
const uint32_t HOLD_THRESHOLD_MS = 1000;  // 1 second hold time

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle effect bypass LED when footswitch is pressed
  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    out[0][i] = !bypass ? 0.0f : in[0][i];
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  led_1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);

    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();

    if (hw.switches[Hothouse::FOOTSWITCH_1].Pressed()) {
      if (footswitch1_start_time == 0) {
        footswitch1_start_time = System::GetNow();
      } else if (System::GetNow() - footswitch1_start_time >=
                 HOLD_THRESHOLD_MS) {
        // Flash LED_1 three times before resetting
        for (int i = 0; i < 3; i++) {
          led_1.Set(1);
          led_1.Update();
          hw.DelayMs(120);
          led_1.Set(0);
          led_1.Update();
          hw.DelayMs(120);
        }
        System::ResetToBootloader();
      }
    } else {
      // Reset the hold timer when footswitch is released
      footswitch1_start_time = 0;
    }
  }

  return 0;
}
