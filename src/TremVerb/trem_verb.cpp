// TremVerb for Hothouse DIY DSP Platform
// Copyright (C) 2024 Cleveland Music Co.
// Contributed by tele_player
// <https://forum.electro-smith.com/u/tele_player/summary> on the Electrosmith Forums
// <https://forum.electro-smith.com/t/hothouse-dsp-pedal-kit/5631/14>
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

// KNOB_1 : trem RATE
// KNOB_2 : trem DEPTH
// KNOB_3 : Reverb Amount
// TOGGLESWITCH_1 : SAW, SIN, SQUARE
// FS_1 : TREM enable
// FS_2 : VERB enable

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisysp::Oscillator;
using daisysp::ReverbSc;
using daisysp::Tremolo;

Hothouse hw;

ReverbSc verb;
Tremolo trem;
Parameter p_rate, p_depth, p_vamt;

// Bypass vars
Led led_trem, led_verb;
bool bypass_trem = true;
bool bypass_verb = true;

int get_waveform(void) {
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case Hothouse::TOGGLESWITCH_UP:
      return Oscillator::WAVE_SAW;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      return Oscillator::WAVE_SIN;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
    default:
      return Oscillator::WAVE_SQUARE;
      break;
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  static float trem_val;
  float vamt;
  hw.ProcessAllControls();

  if (hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
    bypass_trem = !bypass_trem;
  }
  // reduce number of LED Updates for pulsing trem LED
  {
    static int count = 0;
    // set led 100 times/sec
    if (++count == hw.AudioCallbackRate() / 100) {
      count = 0;
      led_trem.Set(bypass_trem ? 0.0f : trem_val);
    }
  }
  led_trem.Update();
  if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
    bypass_verb = !bypass_verb;
    led_verb.Set(bypass_verb ? 0.0f : 1.0f);
  }
  led_verb.Update();

  trem.SetFreq(p_rate.Process());
  trem.SetDepth(p_depth.Process());
  trem.SetWaveform(get_waveform());

  for (size_t i = 0; i < size; ++i) {
    float s, out_l, out_r;
    s = in[0][i];
    if (!bypass_trem) {
      // trem_val gets used above for pulsing LED
      trem_val = trem.Process(1.f);
      s = s * trem_val;
    }
    if (!bypass_verb) {
      verb.Process(s, s, &out_l, &out_r);
      vamt = p_vamt.Process();
      s = (s * (1.0f - vamt) + vamt * ((out_l + out_r) / 2.0f));
    }

    out[0][i] = s;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  // Initialize LEDs
  led_trem.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_verb.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  p_rate.Init(hw.knobs[Hothouse::KNOB_1], 0.2f, 20.0f, Parameter::LINEAR);
  p_depth.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
  p_vamt.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);

  trem.Init(hw.AudioSampleRate());
  trem.SetWaveform(Oscillator::WAVE_SIN);

  verb.Init(hw.AudioSampleRate());
  verb.SetFeedback(0.87);
  verb.SetLpFreq(10000.0f);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (1) {
    ;
  }

  return 0;
}