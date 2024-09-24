// StereoReverbWithTails for Hothouse DIY DSP Platform
// An updated version of the petal/Verb example from the DaisyExamples repo
// Copyright (C) 2024 Cleveland Music Co.
// <code@clevelandmusicco.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "daisysp-lgpl.h"
#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisysp::DelayLine;
using daisysp::ReverbSc;

Hothouse hw;
Parameter v_time, v_freq, v_send, v_pre_delay;
ReverbSc verb;

static DelayLine<float, 48000> DSY_SDRAM_BSS pre_delay_l, pre_delay_r;

Led led_bypass;
bool bypass = true;
bool tail_active = false;
bool mono_to_stereo = false;

float sample_rate;

// Audio callback to process audio samples
void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                   AudioHandle::InterleavingOutputBuffer out, size_t size) {
  float dry_l, dry_r, wet_l, wet_r, send_l, send_r, pre_delayed_l,
      pre_delayed_r;

  hw.ProcessAllControls();
  verb.SetFeedback(v_time.Process());
  verb.SetLpFreq(v_freq.Process());
  v_send.Process();
  v_pre_delay.Process();

  // Set pre-delay based on the knob value
  float pre_delay_time = v_pre_delay.Value();
  pre_delay_l.SetDelay(sample_rate * pre_delay_time);
  pre_delay_r.SetDelay(sample_rate * pre_delay_time);

  // Toggle mono-to-stereo mode
  if (hw.switches[Hothouse::SWITCH_1_UP].Pressed()) {
    mono_to_stereo = true;  // Mono-to-stereo mode is enabled
  } else {
    mono_to_stereo = false;  // Stereo-to-stereo mode is enabled
  }

  // Toggle bypass mode
  if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
    bypass = !bypass;
    if (bypass) tail_active = true;
  }

  for (size_t i = 0; i < size; i += 2) {
    dry_l = in[i];
    dry_r = in[i + 1];

    // Mono-to-stereo mode?
    if (mono_to_stereo) {
      dry_r = dry_l;
    }

    // Pre-delay processing
    pre_delayed_l = pre_delay_l.Read();
    pre_delay_l.Write(dry_l);
    pre_delayed_r = pre_delay_r.Read();
    pre_delay_r.Write(dry_r);

    // Reverb processing
    if (!bypass) {
      send_l = pre_delayed_l * v_send.Value();
      send_r = pre_delayed_r * v_send.Value();
      verb.Process(send_l, send_r, &wet_l, &wet_r);
      out[i] = dry_l + wet_l;
      out[i + 1] = dry_r + wet_r;
    } else if (tail_active) {
      verb.Process(0.0f, 0.0f, &wet_l, &wet_r);
      out[i] = dry_l + wet_l;
      out[i + 1] = dry_r + wet_r;

      // Check if tail has ended
      if (wet_l == 0.0f && wet_r == 0.0f) tail_active = false;
    } else {
      out[i] = dry_l;
      out[i + 1] = dry_r;
    }
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(48);
  sample_rate = hw.AudioSampleRate();

  v_time.Init(hw.knobs[Hothouse::KNOB_1], 0.6f, 0.999f, Parameter::LOGARITHMIC);
  v_freq.Init(hw.knobs[Hothouse::KNOB_2], 500.0f, 20000.0f,
              Parameter::LOGARITHMIC);
  v_send.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  v_pre_delay.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 0.5f, Parameter::LINEAR);

  verb.Init(sample_rate);

  pre_delay_l.Init();
  pre_delay_r.Init();

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (1) {
    hw.DelayMs(10);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    hw.CheckResetToBootloader();
  }
}
