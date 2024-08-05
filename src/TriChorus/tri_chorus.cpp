// TriChorus for Hothouse DIY DSP Platform
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
using daisysp::Chorus;
using daisysp::fonepole;

const int num_voices = 3;
Chorus ch[num_voices];
Hothouse hw;
Led led_bypass;

float wet, vol;
float deltarget[num_voices], del[num_voices];
float lfotarget[num_voices], lfo[num_voices];
bool bypass = true;

float GetVariation(Hothouse::ToggleswitchPosition pos, float up, float middle,
                   float down) {
  switch (pos) {
    case Hothouse::TOGGLESWITCH_UP:
      return up;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      return middle;
    default:
      return down;
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  vol = hw.knobs[Hothouse::KNOB_1].Process() * 2.0f;
  float k = hw.knobs[Hothouse::KNOB_2].Process();
  float base_lfo_freq = k * k * 20.0f;
  float base_lfo_depth = hw.knobs[Hothouse::KNOB_3].Process();
  float base_del = hw.knobs[Hothouse::KNOB_4].Process();
  float feedback = hw.knobs[Hothouse::KNOB_5].Process();
  wet = hw.knobs[Hothouse::KNOB_6].Process();

  // Slight variations for lfo freq, lfo depth, and delay time
  // Tweak to your use case
  float freq_vary = GetVariation(
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1), 0.2f, 0.1f, 0.02f);
  float depth_vary = GetVariation(
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2), 0.4f, 0.2f, 0.05f);
  float delay_vary = GetVariation(
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3), 0.2f, 0.1f, 0.05f);

  for (int i = 0; i < num_voices; ++i) {
    ch[i].SetLfoFreq(base_lfo_freq * (1.0f + freq_vary * (i - 1)));
    lfo[i] = base_lfo_depth * (1.0f + depth_vary * (i - 1));
    del[i] = base_del * (1.0f + delay_vary * (i - 1));
    ch[i].SetFeedback(feedback);
  }

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    float sig = 0.0f;
    for (int j = 0; j < num_voices; ++j) {
      fonepole(del[j], deltarget[j], 0.0001f);  // smooth at audio rate
      ch[j].SetDelay(del[j]);
      fonepole(lfo[j], lfotarget[j], 0.0001f);  // smooth at audio rate
      ch[j].SetLfoDepth(lfo[j]);

      if (!bypass) {
        ch[j].Process(in[0][i]);
        sig += ch[j].GetLeft();
      }
    }
    out[0][i] = bypass ? in[0][i] : (sig * wet + in[0][i] * (1.0f - wet)) * vol;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  for (int i = 0; i < num_voices; ++i) {
    ch[i].Init(hw.AudioSampleRate());
    deltarget[i] = del[i] = 0.0f;
    lfotarget[i] = lfo[i] = 0.0f;
  }

  wet = 0.9f;
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
  }
  return 0;
}
