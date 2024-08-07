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
using daisysp::fclamp;
using daisysp::fonepole;

// Feel free to expirement, but 3 voices seems to work best
const int kNumVoices = 3;

// Typical values for vintage 1980s chorus effects
constexpr float kMinLfoFreq = 0.2f;
constexpr float kMaxLfoFreq = 10.0f;
constexpr float kMinDelay = 1.0f;
constexpr float kMaxDelay = 30.0f;

// Arrays map to toggle switch positions like this:
//   k*Vals[] = {UP, MIDDLE, DOWN}
const float kFreqVals[] = {0.05f, 0.02f, 0.01f};
const float kDepthVals[] = {0.2f, 0.1f, 0.05f};
const float kDelayVals[] = {0.2f, 0.1f, 0.05f};

Chorus ch[kNumVoices];
Hothouse hw;
Led led_bypass;

float wet, vol;
float deltarget[kNumVoices], del[kNumVoices];
float lfotarget[kNumVoices], lfo[kNumVoices];
bool bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  vol = hw.knobs[Hothouse::KNOB_1].Process() * 2.0f;  // Allow output boost

  float base_freq = hw.knobs[Hothouse::KNOB_2].Process();
  base_freq = base_freq * base_freq * kMaxLfoFreq;
  base_freq = fclamp(base_freq, kMinLfoFreq, kMaxLfoFreq);
  float base_depth = hw.knobs[Hothouse::KNOB_3].Process();
  float base_del = hw.knobs[Hothouse::KNOB_4].Process() * kMaxDelay;
  base_del = fclamp(base_del, kMinDelay, kMaxDelay);
  float feedback = hw.knobs[Hothouse::KNOB_5].Process();
  wet = hw.knobs[Hothouse::KNOB_6].Process();

  // Get slight variations for lfo freq, lfo depth, and delay time
  float freq_vary =
      kFreqVals[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)];
  float depth_vary =
      kDepthVals[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)];
  float delay_vary =
      kDelayVals[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3)];

  for (int i = 0; i < kNumVoices; ++i) {
    ch[i].SetLfoFreq(base_freq * (1.0f + freq_vary * (i - 1)));
    lfo[i] = base_depth * (1.0f + depth_vary * (i - 1));
    del[i] = base_del * (1.0f + delay_vary * (i - 1));
    ch[i].SetFeedback(feedback);
  }

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    float sig = 0.0f;
    for (int j = 0; j < kNumVoices; ++j) {
      fonepole(del[j], deltarget[j], 0.0001f);  // smooth at audio rate
      ch[j].SetDelayMs(del[j]);
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

  for (int i = 0; i < kNumVoices; ++i) {
    ch[i].Init(hw.AudioSampleRate());
    deltarget[i] = del[i] = 0.0f;
    lfotarget[i] = lfo[i] = 0.0f;
  }

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
