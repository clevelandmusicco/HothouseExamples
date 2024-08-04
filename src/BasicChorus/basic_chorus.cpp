// BasicChorus for Hothouse DIY DSP Platform
// A port of the petal/chorus example from the DaisyExamples repo
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

// Ported directly from the 'petal/chorus' example in DaisyExamples. Knobs,
// switches, and pins are directly accessed without enums, so look at hothouse.h
// to decipher the mappings. Parameters are not used either; the .Process()
// function on knobs defaults to a 0.0f -> 1.0f range.

// ### It's fair to call this code 'obfuscated', but it's been left as-is. ###

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisysp::Chorus;
using daisysp::fonepole;

Chorus ch;
Hothouse hw;
Led led_bypass;

float wet, vol;
float deltarget, del;
float lfotarget, lfo;
bool bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  wet = hw.knobs[0].Process();
  float k = hw.knobs[1].Process();
  ch.SetLfoFreq(k * k * 20.0f);
  lfo = hw.knobs[2].Process();
  del = hw.knobs[3].Process();
  ch.SetFeedback(hw.knobs[4].Process());
  vol = hw.knobs[5].Process() * 2.0f;

  // footswitch
  bypass ^= hw.switches[7].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    fonepole(del, deltarget, 0.0001f);  // smooth at audio rate
    ch.SetDelay(del);

    fonepole(lfo, lfotarget, 0.0001f);  // smooth at audio rate
    ch.SetLfoDepth(lfo);

    out[0][i] = in[0][i];

    if (!bypass) {
      ch.Process(in[0][i]);
      auto sig = ch.GetLeft();
      if (hw.switches[0].Pressed()) sig += ch.GetRight();  // Sum L+R
      out[0][i] = (sig * wet + in[0][i] * (1.0f - wet)) * vol;
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  ch.Init(hw.AudioSampleRate());

  wet = 0.9f;
  deltarget = del = 0.0f;
  lfotarget = lfo = 0.0f;

  led_bypass.Init(hw.seed.GetPin(23), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
  }
  return 0;
}