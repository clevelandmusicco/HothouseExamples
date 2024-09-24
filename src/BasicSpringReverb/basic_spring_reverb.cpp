// BasicSpringReverb for Hothouse DIY DSP Platform
// A port of the petal/Verb example from the DaisyExamples repo
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

#include "daisysp-lgpl.h"
#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisysp::ReverbSc;

Hothouse hw;
ReverbSc DSY_SDRAM_BSS reverb;
Parameter parm_time, parm_freq, parm_send;
Led led_bypass;
bool bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  float dry, wetl, wetr, send;
  hw.ProcessAllControls();

  reverb.SetFeedback(parm_time.Process());
  reverb.SetLpFreq(parm_freq.Process());
  parm_send.Process();

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    dry = in[0][i];
    send = dry * parm_send.Value();
    reverb.Process(send, send, &wetl, &wetr);

    if (bypass) {
      out[0][i] = out[1][i] = in[0][i];
    } else {
      out[0][i] = dry + wetl;
      out[1][i] = dry + wetr;
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float samplerate = hw.AudioSampleRate();

  parm_time.Init(hw.knobs[Hothouse::KNOB_1], 0.6f, 0.999f,
                 Parameter::LOGARITHMIC);
  parm_freq.Init(hw.knobs[Hothouse::KNOB_2], 500.0f, 20000.0f,
                 Parameter::LOGARITHMIC);
  parm_send.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  reverb.Init(samplerate);

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    hw.CheckResetToBootloader();
  }
  return 0;
}