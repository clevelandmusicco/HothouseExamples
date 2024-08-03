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

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisysp::Tremolo;
using daisysp::Oscillator;

Hothouse hw;
Tremolo trem;
Led led_bypass;

int waveform;
bool bypass = true;

// An oasis of readable code ;) 
int GetWaveform() {
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case Hothouse::TOGGLESWITCH_UP:
      return Oscillator::WAVE_POLYBLEP_TRI;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      return Oscillator::WAVE_SIN;
    case Hothouse::TOGGLESWITCH_DOWN:
    default:
      return Oscillator::WAVE_POLYBLEP_SQUARE;
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  trem.SetFreq(hw.knobs[0].Process() * 20.f);  // 0 - 20 Hz
  trem.SetDepth(hw.knobs[1].Process());
  trem.SetWaveform(GetWaveform());

  bypass ^= hw.switches[7].RisingEdge();
  led_bypass.Set(bypass ? 0.0f : 1.0f);
  led_bypass.Update();

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
    // Do nothing forever
  }
  return 0;
}