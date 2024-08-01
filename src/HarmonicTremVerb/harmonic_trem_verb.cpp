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

Hothouse hw;
OnePole lpf, hpf;
ExtendedOscillator osc1, osc2;
ReverbSc reverb;

Parameter parm_trem_freq, parm_trem_depth, parm_trem_lpf_cutoff,
    parm_trem_hpf_cutoff, parm_reverb_send, parm_reverb_feedback;
Led led_trem_bypass, led_reverb_bypass;

bool trem_bypass = true;
bool reverb_bypass = true;

int GetWaveform() {
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)) {
    case Hothouse::TOGGLESWITCH_UP:
      return ExtendedOscillator::WAVE_TRI;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      return ExtendedOscillator::WAVE_SIN;
    case Hothouse::TOGGLESWITCH_DOWN:
    default:
      return ExtendedOscillator::WAVE_SQUARE_ROUNDED;
  }
}

void ToggleBypass(bool &bypass, Led &led) {
  bypass = !bypass;
  led.Set(bypass ? 0.0f : 1.0f);
}

void UpdateBypassStates() {
  if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
    ToggleBypass(trem_bypass, led_trem_bypass);
  }

  if (hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
    ToggleBypass(reverb_bypass, led_reverb_bypass);
  }
}

void SetFilterFrequencies(float sample_rate) {
  float lpf_freq = parm_trem_lpf_cutoff.Process() / sample_rate;
  float hpf_freq = parm_trem_hpf_cutoff.Process() / sample_rate;
  lpf.SetFrequency(lpf_freq);
  hpf.SetFrequency(hpf_freq);
}

void SetOscillatorParameters() {
  float freq = parm_trem_freq.Process();
  float depth = parm_trem_depth.Process();
  osc1.SetFreq(freq);
  osc2.SetFreq(freq);
  osc1.SetAmp(depth);
  osc2.SetAmp(depth);
  int waveform = GetWaveform();
  osc1.SetWaveform(waveform);
  osc2.SetWaveform(waveform);
}

void SetReverbParameters() {
  reverb.SetFeedback(parm_reverb_feedback.Process());
  parm_reverb_send.Process();
}

void SetReverbDamping() {
  float lp_freq;
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case Hothouse::TOGGLESWITCH_UP:
      lp_freq = 10000.0f;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      lp_freq = 5000.0f;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
    default:
      lp_freq = 2500.0f;
      break;
  }
  reverb.SetLpFreq(lp_freq);
}

void ProcessAudio(float dry, float &proc) {
  float send, wet_l, wet_r;

  if (!reverb_bypass) {
    send = dry * parm_reverb_send.Value();
    reverb.Process(send, send, &wet_l, &wet_r);
    proc = dry + (wet_l + wet_r / 2.0f);
  }

  if (!trem_bypass) {
    if (hw.switches[Hothouse::SWITCH_3_UP].Pressed()) {
      proc = (lpf.Process(proc) * (1 - osc1.Process())) +
             (hpf.Process(proc) * (1 - osc2.Process()));
    } else {
      proc = proc * (1 - osc1.Process());
    }
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();
  UpdateBypassStates();
  float sample_rate = hw.AudioSampleRate();
  SetFilterFrequencies(sample_rate);
  SetOscillatorParameters();
  SetReverbParameters();
  SetReverbDamping();

  for (size_t i = 0; i < size; ++i) {
    float dry = in[0][i];
    float proc = dry;
    ProcessAudio(dry, proc);
    out[0][i] = proc;
  }

  led_reverb_bypass.Update();
  led_trem_bypass.Update();
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

  parm_reverb_send.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f,
                        Parameter::LINEAR);
  parm_trem_freq.Init(hw.knobs[Hothouse::KNOB_2], 0.2f, 12.0f,
                      Parameter::LINEAR);
  parm_trem_depth.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f,
                       Parameter::LINEAR);
  parm_reverb_feedback.Init(hw.knobs[Hothouse::KNOB_4], 0.6f, 1.0f,
                            Parameter::LINEAR);

  // More arbitrary magic numbers; these set the upper and lower bounds
  // of the LPF and HPF applied to the "split" signal before amplitude
  // modulation ... tune to taste
  parm_trem_lpf_cutoff.Init(hw.knobs[Hothouse::KNOB_5], 100.0f, 500.f,
                            Parameter::LINEAR);
  parm_trem_hpf_cutoff.Init(hw.knobs[Hothouse::KNOB_6], 500.0f, 1000.f,
                            Parameter::LINEAR);

  lpf.Init();
  lpf.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
  hpf.Init();
  hpf.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
  osc1.Init(hw.AudioSampleRate());

  // For the harmonic tremolo effect, set the second oscillator
  // 180 degrees out of phase with the first
  osc2.Init(hw.AudioSampleRate());
  osc2.PhaseAdd(0.5);

  reverb.Init(hw.AudioSampleRate());

  led_reverb_bypass.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_reverb_bypass.Update();

  led_trem_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);
  led_trem_bypass.Update();

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // Infinite loop to keep the program running
  }

  return 0;
}