// StereoTest for Hothouse DIY DSP Platform
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
using daisy::Parameter;
using daisy::SaiHandle;
using daisysp::fonepole;
using daisysp::Oscillator;

Hothouse hw;
Oscillator osc;

Parameter p_freq, p_amp;
Led led_bypass;
bool bypass = true;

// Frequency and amplitude ranges
const float kFreqMin = 20.0f;
const float kFreqMax = 20000.0f;
const float kAmpMin = 0.0f;
const float kAmpMax = 0.5f;

// Crossfade-related variables
float left_gain = 1.0f, right_gain = 0.0f;
const float kSmoothingFactor = 0.0001f;

// Waveform options in an array
const int kNumWaveforms = 4;
const int waveforms[kNumWaveforms] = {
    Oscillator::WAVE_SIN, Oscillator::WAVE_TRI, Oscillator::WAVE_SAW,
    Oscillator::WAVE_SQUARE};

void SetOscillatorWaveform(Oscillator &osc, float knob_value) {
  // Map knob value to a waveform index and set the waveform
  int waveform_index = static_cast<int>(knob_value * kNumWaveforms);
  waveform_index =
      (waveform_index >= kNumWaveforms) ? kNumWaveforms - 1 : waveform_index;
  osc.SetWaveform(waveforms[waveform_index]);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();
  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  osc.SetFreq(p_freq.Process());
  osc.SetAmp(p_amp.Process());

  float waveform_knob_value = hw.knobs[Hothouse::KNOB_3].Process();
  SetOscillatorWaveform(osc, waveform_knob_value);

  float target_left_gain =
      (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1) !=
       Hothouse::TOGGLESWITCH_DOWN)
          ? 1.0f
          : 0.0f;
  float target_right_gain =
      (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1) !=
       Hothouse::TOGGLESWITCH_UP)
          ? 1.0f
          : 0.0f;

  for (size_t i = 0; i < size; ++i) {
    // Smooth at audio rate
    fonepole(left_gain, target_left_gain, kSmoothingFactor);
    fonepole(right_gain, target_right_gain, kSmoothingFactor);

    if (bypass) {
      out[0][i] = out[1][i] = 0.0f;  // Silence when bypassed
    } else {
      float signal = osc.Process();
      out[0][i] = signal * left_gain;
      out[1][i] = signal * right_gain;
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  p_freq.Init(hw.knobs[Hothouse::KNOB_1], kFreqMin, kFreqMax,
              Parameter::LOGARITHMIC);
  p_amp.Init(hw.knobs[Hothouse::KNOB_2], kAmpMin, kAmpMax, Parameter::LINEAR);

  osc.Init(hw.AudioSampleRate());
  osc.SetWaveform(Oscillator::WAVE_SIN);

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
