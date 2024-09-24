// BasicPhaser for Hothouse DIY DSP Platform
// A port of the petal/phaser example from the DaisyExamples repo
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

// Ported directly from the 'petal/phaser' example in DaisyExamples. Knobs,
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
using daisysp::fonepole;
using daisysp::Phaser;

Hothouse hw;
Phaser phaser;
Led led_bypass;

float wet;
float freqtarget, freq;
float lfotarget, lfo;
bool bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // knobs
  wet = hw.knobs[0].Process();
  float k = hw.knobs[1].Process();
  phaser.SetLfoFreq(k * k * 20.f);
  lfo = hw.knobs[2].Process();
  k = hw.knobs[3].Process();
  freq = k * k * 7000;  // 0 - 10 kHz, square curve
  phaser.SetFeedback(hw.knobs[4].Process());

  // simulate encoder in original example
  phaser.SetPoles(1 + static_cast<int>(hw.knobs[5].Process() * 7.0f));

  // footswitch
  bypass ^= hw.switches[7].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    fonepole(freq, freqtarget, 0.0001f);  // smooth at audio rate
    phaser.SetFreq(freq);

    fonepole(lfo, lfotarget, 0.0001f);  // smooth at audio rate
    phaser.SetLfoDepth(lfo);

    // Copy left input to both outputs (dual mono)
    out[0][i] = out[1][i] = in[0][i];

    if (!bypass) {
      out[0][i] = out[1][i] =
          phaser.Process(in[0][i]) * wet + in[0][i] * (1.f - wet);
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float sample_rate = hw.AudioSampleRate();

  phaser.Init(sample_rate);

  wet = 0.9f;
  freqtarget = freq = 0.0f;
  lfotarget = lfo = 0.0f;

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    hw.CheckResetToBootloader();
  }
  return 0;
}