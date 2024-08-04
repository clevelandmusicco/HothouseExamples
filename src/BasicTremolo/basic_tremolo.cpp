// BasicTremolo for Hothouse DIY DSP Platform
// Copyright (C) 2024 Your Name <your@email>
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

// Ported directly from the 'petal/tremolo' example in DaisyExamples. Knobs,
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
using daisysp::Oscillator;
using daisysp::Tremolo;

Hothouse hw;
Led led_bypass;
Tremolo trem;

int waveform;
bool bypass = true;

int GetWaveform() {
  // lookup array for brevity
  static const int waveforms[] = {Oscillator::WAVE_POLYBLEP_TRI,
                                  Oscillator::WAVE_SIN,
                                  Oscillator::WAVE_POLYBLEP_SQUARE};
  return waveforms[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)];
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  trem.SetFreq(hw.knobs[0].Process() * 20.f);  // 0 - 20 Hz
  trem.SetDepth(hw.knobs[1].Process());
  trem.SetWaveform(GetWaveform());

  // footswitch
  bypass ^= hw.switches[7].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    out[0][i] = !bypass ? trem.Process(in[0][i]) : in[0][i];
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float sample_rate = hw.AudioSampleRate();

  trem.Init(sample_rate);
  waveform = 0;

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