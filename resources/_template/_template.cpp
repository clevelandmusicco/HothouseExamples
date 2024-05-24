// @@template_uc for Hothouse DIY DSP Platform
// Copyright (C) 2024 @@your_name <@@your_email>
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

Hothouse hw;

// Bypass vars
Led led_bypass;
bool bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle effect bypass LED when footswitch is pressed
  if (hw.switches[Hothouse::FOOTSWITCH_2].FallingEdge()) {
    bypass = !bypass;
    // LED off when bypassed, on otherwise
    led_bypass.Set(bypass ? 0.0f : 1.0f);
  }

  // Update the LEDs
  led_bypass.Update();

  for (size_t i = 0; i < size; ++i) {
    if (bypass) {
      out[0][i] = in[0][i];
    } else {
      out[0][i] = 0.0f;  // TODO: replace silence with something awesome
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  // Initialize LEDs
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // Do nothing forever
  }
  return 0;
}