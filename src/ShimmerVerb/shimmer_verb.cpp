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

// DISCLAIMER: Experienced DSP developers will know there are much more
// sophisticated, elegant, or "better" ways to do a lot of the things this
// code does. However, to make it readily accessible to hackers of all
// experience levels, the code is straightforward and easy to follow by design.
// If you feel strongly that the former outweighs the latter, please feel
// free to submit a pull request to get your improvements into the codebase.

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
OnePole ar_lpf, ar_hpf, ps_lpf;
Parameter decay_param, lpf_param, mix_param, depth_param, rate_param,
    shimmer_param;
PitchShifter ps;
ReverbSc DSY_SDRAM_BSS reverb;

// Bypass state
Led led_bypass;
bool bypass;

// 100% wet state
Led led_hundred_percent_wet;
bool hundred_percent_wet;

DelayLine<float, 48000> DSY_SDRAM_BSS delay;
float delay_time;
float feedback;
float sample_rate;

// Audio callback function, processes audio in real-time
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle effect bypass LED when footswitch is pressed
  if (hw.switches[Hothouse::FOOTSWITCH_2].FallingEdge()) {
    bypass = !bypass;
    // LED off when bypassed, on otherwise
    led_bypass.Set(bypass ? 0.0f : 1.0f);
  }

  // Toggle 100% wet LED when footswitch is pressed
  if (hw.switches[Hothouse::FOOTSWITCH_1].FallingEdge()) {
    hundred_percent_wet = !hundred_percent_wet;
    // LED on when in "100% wet" mode, off otherwise
    led_hundred_percent_wet.Set(hundred_percent_wet ? 1.0f : 0.0f);
  }

  // Get parameter values
  float decay = decay_param.Process();
  float lpf = lpf_param.Process();
  float mix = mix_param.Process();
  float depth = depth_param.Process();
  float rate = rate_param.Process();
  float shimmer = shimmer_param.Process();

  // Set effect parameters
  reverb.SetFeedback(decay);
  reverb.SetLpFreq(lpf);
  chorus.SetLfoDepth(depth);
  chorus.SetLfoFreq(rate);
  chorus.SetFeedback(0);
  ps.SetDelSize(4200);  // Delay in samples
  ps.SetFun(0.05f);     // Adds some warbly texture to the pitch-shifting
  ps_lpf.SetFrequency(2200.0f / sample_rate);  // Set pitch shifter LPF

  // Determine pitch shift interval based on TOGGLESWITCH_1
  float semitones = 0.0f;
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case Hothouse::TOGGLESWITCH_UP:
      semitones = 19.0f;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      semitones = 12.0f;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
      semitones = 7.0f;
      break;
    default:
      break;
  }
  ps.SetTransposition(semitones);

  // Set filter frequencies based on TOGGLESWITCH_2 for Abbey Road reverb trick
  float hpf_freq, lpf_freq;
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)) {
    case Hothouse::TOGGLESWITCH_UP:
      hpf_freq = 600.0f;
      lpf_freq = 6000.0f;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      hpf_freq = 600.0f;
      lpf_freq = 10000.0f;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
      hpf_freq = 24.0f;
      lpf_freq = 22000.0f;
      break;
    default:
      hpf_freq = 24.0f;
      lpf_freq = 22000.0f;
      break;
  }
  ar_hpf.SetFrequency(hpf_freq / sample_rate);
  ar_lpf.SetFrequency(lpf_freq / sample_rate);

  // Process audio samples
  for (size_t i = 0; i < size; i++) {
    if (bypass) {
      out[0][i] = in[0][i];
      continue;
    }

    float dry = in[0][i];
    float filtered = ar_lpf.Process(ar_hpf.Process(dry));
    float pitch_shifted = ps_lpf.Process(ps.Process(filtered));
    float send = (filtered + pitch_shifted * shimmer) *
                 (hundred_percent_wet ? 1.0f : mix);
    float wet;

    reverb.Process(send, send, &wet, &wet);

    // Determine delay settings based on TOGGLESWITCH_3
    bool apply_delay = false;
    switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3)) {
      case Hothouse::TOGGLESWITCH_UP:
        delay_time = 0.3f;
        feedback = decay * 0.6f;
        apply_delay = true;
        break;
      case Hothouse::TOGGLESWITCH_MIDDLE:
        delay_time = 0.15f;
        feedback = 0.3f;
        apply_delay = true;
        break;
      case Hothouse::TOGGLESWITCH_DOWN:
        break;
      default:
        break;
    }

    if (apply_delay) {
      delay.SetDelay(sample_rate * delay_time);
      wet += delay.Read();
    }

    if (depth >= 0.1f) {
      float chorused = chorus.Process(wet);
      wet = (wet + chorused * 2.0f) / 2.0f;
    }

    out[0][i] = hundred_percent_wet ? wet : dry + wet;

    // Write back to the circular buffer
    delay.Write(wet * feedback);
  }

  // Update LEDs
  led_bypass.Update();
  led_hundred_percent_wet.Update();
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sample_rate = hw.AudioSampleRate();

  // Init parameters with associated hardware controls
  decay_param.Init(hw.knobs[Hothouse::KNOB_1], 0.3f, 0.99f, Parameter::LINEAR);
  lpf_param.Init(hw.knobs[Hothouse::KNOB_2], 600.0f, 22000.0f,
                 Parameter::LINEAR);
  mix_param.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  depth_param.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  rate_param.Init(hw.knobs[Hothouse::KNOB_5], 0.1f, 5.0f, Parameter::LINEAR);
  shimmer_param.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  // Init effects
  chorus.Init(sample_rate);
  delay.Init();
  ps.Init(sample_rate);
  reverb.Init(sample_rate);

  // Helper lambda to init filters
  auto init_filter = [](OnePole& filter, OnePole::FilterMode mode) {
    filter.Init();
    filter.SetFilterMode(mode);
  };

  init_filter(ar_hpf, OnePole::FILTER_MODE_HIGH_PASS);
  init_filter(ar_lpf, OnePole::FILTER_MODE_LOW_PASS);
  init_filter(ps_lpf, OnePole::FILTER_MODE_LOW_PASS);

  // Helper lambda to init LEDs
  auto init_led = [](Led& led, dsy_gpio_pin pin) {
    led.Init(pin, false);
    led.Update();
  };

  init_led(led_bypass, hw.seed.GetPin(Hothouse::LED_2));
  init_led(led_hundred_percent_wet, hw.seed.GetPin(Hothouse::LED_1));

  bypass = true;  // Initial bypass state
  hundred_percent_wet = false;

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // Main loop runs indefinitely
  }

  return 0;
}
