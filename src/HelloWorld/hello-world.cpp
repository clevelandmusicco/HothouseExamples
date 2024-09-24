// Hello World for Hothouse DIY DSP Platform
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

// Needed for some VS Code configs; uncomment if ReverbSc
// and other LGPL stuff is not recognized by Intellisense ¯\_(ツ)_/¯
// #include "daisysp-lgpl.h"
#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;

Hothouse hw;

// LEDs for bypass and switch example
Led led_1, led_bypass;
bool led_1_on = false;
bool bypass = true;

Parameter parm_bright;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle effect bypass LED when footswitch is pressed
  if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
    bypass = !bypass;
    // LED off when bypassed, on otherwise
    led_bypass.Set(bypass ? 0.0f : 1.0f);
  }

  // An example of how to read the position of the switches
  // This code lights LED_1 if TOGGLESWITCH_1 is either UP or DOWN;
  // LED_1 is unlit if TOGGLESWITCH_1 is in the MIDDLE
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case (Hothouse::TOGGLESWITCH_UP):
    case (Hothouse::TOGGLESWITCH_DOWN):
      led_1_on = true;
      break;
    case (Hothouse::TOGGLESWITCH_MIDDLE):
      // TOGGLESWITCH_MIDDLE requires an ON-OFF-ON switch.
      // See project wiki for more information.
      led_1_on = false;
      break;
    default:
      led_1_on = false;
  }

  // When lit, scale LED_1 brightness with KNOB_1 (assigned to parm_bright)
  led_1.Set(led_1_on ? parm_bright.Process() * 1.0f : 0.0f);

  // Update the LEDs
  led_1.Update();
  led_bypass.Update();

  // Audio processing; since this is a HelloWorld,
  // just pass the input to the output in either state
  for (size_t i = 0; i < size; ++i) {
    if (bypass) {
      out[0][i] = out[1][i] = in[0][i];
    } else {
      out[0][i] = out[1][i] = in[0][i];  // TODO: replace with something awesome
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  // We'll use KNOB_1 to control brightness of LED_1
  parm_bright.Init(hw.knobs[Hothouse::KNOB_1], 0.3f, 1.0f, Parameter::LINEAR);

  // Initialize LEDs
  led_1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    hw.CheckResetToBootloader();
  }
  return 0;
}