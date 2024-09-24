// Harmonic tremolo and reverb effect for Hothouse DIY DSP Pedal
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

#include "daisysp-lgpl.h"  // Necessary for some VS Code configs
#include "daisysp.h"
#include "extended_oscillator.h"
#include "hothouse.h"

using clevelandmusicco::ExtendedOscillator;
using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisysp::OnePole;
using daisysp::ReverbSc;

// Array maps to toggle switch positions like this:
//   kVerbLpVals[] = {UP, MIDDLE, DOWN}
constexpr float kVerbLpVals[] = {10000.0f, 5000.0f, 2500.0f};
constexpr int kWaveformMap[] = {
    ExtendedOscillator::WAVE_TRI,            // UP
    ExtendedOscillator::WAVE_SIN,            // MIDDLE
    ExtendedOscillator::WAVE_SQUARE_ROUNDED  // DOWN
};

const float kLpfCutoffLower = 100.0f;
const float kLpfCutoffUpper = 600.0f;
const float kHpfCutoffLower = 450.0f;
const float kHpfCutoffUpper = 1000.0f;

Hothouse hw;
OnePole lpf, hpf;
ExtendedOscillator osc;
ReverbSc reverb;

Parameter p_trem_freq, p_trem_depth, p_trem_lpf_cutoff, p_trem_hpf_cutoff,
    p_verb_send, p_verb_fdbk;
Led led_trem_bypass, led_reverb_bypass;

bool trem_bypass = true;
bool reverb_bypass = true;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  trem_bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();
  reverb_bypass ^= hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge();

  float sample_rate = hw.AudioSampleRate();

  // Filters used for split harmonic trem signal paths
  lpf.SetFrequency(p_trem_lpf_cutoff.Process() / sample_rate);
  hpf.SetFrequency(p_trem_hpf_cutoff.Process() / sample_rate);

  float freq = p_trem_freq.Process();
  float depth = p_trem_depth.Process();
  osc.SetFreq(freq);
  osc.SetAmp(depth);

  // Toggleswitch 2 selects trem waveform
  osc.SetWaveform(
      kWaveformMap[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)]);

  reverb.SetFeedback(p_verb_fdbk.Process());
  p_verb_send.Process();
  reverb.SetLpFreq(
      kVerbLpVals[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)]);

  for (size_t i = 0; i < size; ++i) {
    float dry = in[0][i];
    float proc_l = dry, proc_r = dry;

    // Process audio with reverb
    if (!reverb_bypass) {
      float send = dry * p_verb_send.Value();
      float wet_l, wet_r;
      reverb.Process(send, send, &wet_l, &wet_r);
      proc_l = dry + wet_l;
      proc_r = dry + wet_r;
    }

    // Process audio with tremolo
    if (!trem_bypass) {
      auto freq_factor = 2 * M_PI / sample_rate;
      auto adj_factor = pow(sin(freq_factor * freq), 2);
      auto osc_sig = osc.Process();

      // Apply tremolo effect
      auto apply_trem = [&](float signal, float osc_sig) {
        return signal * (1 - osc_sig) + osc_sig * adj_factor;
      };

      float inverted_osc_sig = -osc_sig;

      if (hw.switches[Hothouse::SWITCH_3_UP].Pressed() ||
          hw.switches[Hothouse::SWITCH_3_DOWN].Pressed()) {
        bool is_up = hw.switches[Hothouse::SWITCH_3_UP].Pressed();

        proc_l =
            apply_trem(lpf.Process(proc_l), osc_sig) +
            apply_trem(hpf.Process(proc_l), is_up ? inverted_osc_sig : osc_sig);

        proc_r =
            apply_trem(lpf.Process(proc_r),
                       is_up ? osc_sig : inverted_osc_sig) +
            apply_trem(hpf.Process(proc_r), is_up ? inverted_osc_sig : osc_sig);
      } else {
        proc_l = apply_trem(proc_l, osc_sig);
        proc_r = apply_trem(proc_r, osc_sig);
      }
    }

    // mono-to-stereo reverb, trem is still dual-mono
    out[0][i] = proc_l;
    out[1][i] = proc_r;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

  p_verb_send.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 0.99f, Parameter::LINEAR);
  p_trem_freq.Init(hw.knobs[Hothouse::KNOB_2], 0.1f, 12.0f, Parameter::LINEAR);
  p_trem_depth.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  p_verb_fdbk.Init(hw.knobs[Hothouse::KNOB_4], 0.6f, 1.0f, Parameter::LINEAR);
  p_trem_lpf_cutoff.Init(hw.knobs[Hothouse::KNOB_5], kLpfCutoffLower,
                         kLpfCutoffUpper, Parameter::LINEAR);
  p_trem_hpf_cutoff.Init(hw.knobs[Hothouse::KNOB_6], kHpfCutoffLower,
                         kHpfCutoffUpper, Parameter::LINEAR);

  lpf.Init();
  lpf.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
  hpf.Init();
  hpf.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
  osc.Init(hw.AudioSampleRate());

  reverb.Init(hw.AudioSampleRate());

  led_reverb_bypass.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_reverb_bypass.Update();

  led_trem_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);
  led_trem_bypass.Update();

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(6);
    led_reverb_bypass.Set(reverb_bypass ? 0.0f : 1.0f);
    led_reverb_bypass.Update();
    led_trem_bypass.Set(trem_bypass ? 0.0f : 1.0f);
    led_trem_bypass.Update();
    hw.CheckResetToBootloader();
  }

  return 0;
}
