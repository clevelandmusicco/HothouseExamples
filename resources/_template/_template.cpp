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

// ### Uncomment if IntelliSense can't resolve DaisySP-LGPL classes ###
// #include "daisysp-lgpl.h"

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;

Hothouse hw;

Led led_bypass;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle bypass when FOOTSWITCH_2 is pressed. Use the accessor, not
  // hw.switches[] -- the accessor also picks up MIDI CC 24.
  if (hw.GetFootswitchRisingEdge(Hothouse::FOOTSWITCH_2)) {
    hw.ToggleBypass();
  }
  const bool bypass = hw.GetBypass();

  for (size_t i = 0; i < size; ++i) {
    if (bypass) {
      // Copy left input to both outputs (mono-to-dual-mono)
      out[0][i] = out[1][i] = in[0][i];
    } else {
      // TODO: replace silence with something awesome
      out[0][i] = out[1][i] = 0.0f;
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  // Enables the built-in MIDI CC map (CC 14-25). Claims the internal USB
  // peripheral, so drop this line if you'd rather have seed.StartLog().
  hw.StartMidi();
  hw.SetBypass(true);

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  // 1 ms, not the usual 10, so a dense CC stream can't back up in the USB
  // MIDI FIFO; ProcessMidi() is the only place the queue gets drained.
  while (true) {
    hw.ProcessMidi();
    hw.DelayMs(1);

    // Toggle effect bypass LED when footswitch is pressed
    led_bypass.Set(hw.GetBypass() ? 0.0f : 1.0f);
    led_bypass.Update();

    // Call System::ResetToBootloader() if FOOTSWITCH_1 is pressed for 2 seconds
    hw.CheckResetToBootloader();
  }
  return 0;
}