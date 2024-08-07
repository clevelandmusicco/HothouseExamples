// ShimmerVerb for Hothouse DIY DSP Platform
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
using daisysp::ChorusEngine;
using daisysp::DelayLine;
using daisysp::OnePole;
using daisysp::PitchShifter;
using daisysp::ReverbSc;

// Global variables for hardware and effects parameters
ChorusEngine chorus;
Hothouse hw;
OnePole ar_lpf, ar_hpf, ps_lpf, ps_hpf;
Parameter p_decay, p_lpf, p_mix, p_depth, p_rate, p_shimmer;
PitchShifter ps;
ReverbSc DSY_SDRAM_BSS reverb;

Led led_bypass, led_hundred_percent_wet;
bool bypass, hundred_percent_wet;

DelayLine<float, 48000> DSY_SDRAM_BSS delay;
float delay_time, feedback, sample_rate;

// Arrays map to toggle switch positions like this:
//   *_values[] = {UP, MIDDLE, DOWN}
const float kSemitoneVals[] = {19.0f, 12.0f, 7.0f};
const float kHpfVals[] = {600.0f, 600.0f, 24.0f};
const float kLpfVals[] = {6000.0f, 10000.0f, 22000.0f};
const float kDelayTimeVals[] = {0.3f, 0.15f, 0.0f};
const float kFeedbackVals[] = {0.6f, 0.3f, 0.0f};

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();
  hundred_percent_wet ^= hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge();

  float decay = p_decay.Process();
  float lpf = p_lpf.Process();
  float mix = p_mix.Process();
  float depth = p_depth.Process();
  float rate = p_rate.Process();
  float shimmer = p_shimmer.Process();

  reverb.SetFeedback(decay);
  reverb.SetLpFreq(lpf);
  chorus.SetLfoDepth(depth);
  chorus.SetLfoFreq(rate);
  chorus.SetFeedback(0);
  ps.SetDelSize(0.085f * sample_rate);
  ps.SetFun(0.03f);
  ps_lpf.SetFrequency(4000.0f / sample_rate);
  ps_hpf.SetFrequency(600.0f / sample_rate);

  int sw1_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
  ps.SetTransposition(kSemitoneVals[sw1_pos]);

  int sw2_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
  ar_hpf.SetFrequency(kHpfVals[sw2_pos] / sample_rate);
  ar_lpf.SetFrequency(kLpfVals[sw2_pos] / sample_rate);

  int sw3_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
  delay_time = kDelayTimeVals[sw3_pos];
  feedback = kFeedbackVals[sw3_pos];
  bool apply_delay = delay_time > 0;

  for (size_t i = 0; i < size; i++) {
    if (bypass) {
      out[0][i] = in[0][i];
      continue;
    }

    float dry = in[0][i];
    float filtered = ar_lpf.Process(ar_hpf.Process(dry));
    float pre_ps = ps_hpf.Process(dry);
    float pitch_shifted = ps_lpf.Process(ps.Process(pre_ps));
    float send = (filtered + pitch_shifted * shimmer) *
                 (hundred_percent_wet ? 1.0f : mix);
    float wet;

    reverb.Process(send, send, &wet, &wet);

    if (apply_delay) {
      delay.SetDelay(sample_rate * delay_time);
      wet += delay.Read();
    }

    if (depth >= 0.1f) {
      float chorused = chorus.Process(wet);
      wet = chorused * 1.5f;
    }

    out[0][i] = hundred_percent_wet ? wet : dry + wet;
    delay.Write(wet * feedback);
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sample_rate = hw.AudioSampleRate();

  p_decay.Init(hw.knobs[Hothouse::KNOB_1], 0.3f, 0.99f, Parameter::LINEAR);
  p_lpf.Init(hw.knobs[Hothouse::KNOB_2], 600.0f, 22000.0f, Parameter::LINEAR);
  p_mix.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  p_depth.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 0.7f, Parameter::LINEAR);
  p_rate.Init(hw.knobs[Hothouse::KNOB_5], 0.1f, 5.0f, Parameter::LINEAR);
  p_shimmer.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  chorus.Init(sample_rate);
  delay.Init();
  ps.Init(sample_rate);
  reverb.Init(sample_rate);

  auto init_filter = [](OnePole& filter, OnePole::FilterMode mode) {
    filter.Init();
    filter.SetFilterMode(mode);
  };

  init_filter(ar_hpf, OnePole::FILTER_MODE_HIGH_PASS);
  init_filter(ar_lpf, OnePole::FILTER_MODE_LOW_PASS);
  init_filter(ps_lpf, OnePole::FILTER_MODE_LOW_PASS);
  init_filter(ps_hpf, OnePole::FILTER_MODE_HIGH_PASS);

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);
  led_hundred_percent_wet.Init(hw.seed.GetPin(Hothouse::LED_1), false);

  bypass = true;
  hundred_percent_wet = false;

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    led_hundred_percent_wet.Set(hundred_percent_wet ? 1.0f : 0.0f);
    led_hundred_percent_wet.Update();
  }

  return 0;
}
