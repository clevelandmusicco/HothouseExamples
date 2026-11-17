// Neptune for Hothouse DIY DSP Platform
// Copyright (C) 2026 Ricky Sheaves <code@clevelandmusicco.com>
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

// Neptune: stereo reverb (CloudSeed) plus normal / octave / reverse delay.
//
// Port of GuitarML's Neptune for the Funbox, MIT License,
// Copyright (c) 2024 Keith Bloemer. See LICENSE-GuitarML.
//   https://github.com/GuitarML/FunBox/tree/main/software/Neptune
//
// Third-party code in this directory (MIT; notices kept in each file):
//   cloudseed/           CloudSeed by Valdemar Erlingsson, Daisy port by
//                        Erwin Coumans, Daisy changes by GuitarML
//   delayline_reverse.h  Veno-Echo reverse delay by Adam Fulford, GuitarML mods
//   delayline_oct.h      DaisySP DelayLine (Electrosmith), GuitarML octave mods
//
// A faithful port: the DSP is GuitarML's, changed only where the Hothouse
// needed it or where the original had a bug:
//   - FOOTSWITCH 1 holds alternate mode, FOOTSWITCH 2 is bypass (with trails)
//   - MIDI, expression, presets and dip switches removed; stereo input is
//     the compile-time NEPTUNE_TRUE_STEREO=1 (default: left in, stereo out)
//   - Octave-delay lowpass is per channel (the original shared one L/R)
//   - Reverb program changes mute the wet path on both channels, and
//     re-apply KNOB 5 along with the other knobs
//   - CloudSeed pool allocations are 8-byte aligned
// See README.md for controls.

#include <cmath>

#include "AudioLib/ValueTables.h"
#include "Default.h"
#include "FastSin.h"
#include "ReverbController.h"
#include "daisysp.h"
#include "delayline_oct.h"
#include "delayline_reverse.h"
#include "hothouse.h"

#ifndef NEPTUNE_TRUE_STEREO
#define NEPTUNE_TRUE_STEREO 0
#endif

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisysp::fonepole;
using daisysp::Tone;

Hothouse hw;
Led led_alt, led_bypass;

daisy::Parameter p_decay, p_mix, p_delay_time, p_mod, p_filter, p_feedback;

bool bypass = true;
bool alternate_mode = false;
bool force_reset = true;  // re-apply knob params without waiting for movement

// Input-to-effect gain: 1 engaged, 0 bypassed. Crossfades over ~5 ms so
// trails bypass doesn't write a step into the delay and reverb.
float engage = 0.0f;

float dry_mix = 0.0f;
float wet_mix = 0.0f;
float delay_mix = 0.703f;  // set by the alternate function of KNOB 2
float reverb_mix = 0.703f;

// Last applied knob values, for the move-to-update hysteresis
float p_decay_last = 0.0f;
float p_mix_last = 0.5f;
float p_mod_last = 0.0f;
float p_filter_last = 0.5f;

// --- Reverb ---

CloudSeed::ReverbController* reverb = nullptr;

// CloudSeed carves its buffers from this via custom_pool_allocate()
constexpr size_t kPoolSize = 48 * 1024 * 1024;
DSY_SDRAM_BSS char custom_pool[kPoolSize];
size_t pool_index = 0;

void* custom_pool_allocate(size_t size) {
  size = (size + 7) & ~static_cast<size_t>(7);  // keep doubles aligned
  if (pool_index + size >= kPoolSize) {
    return nullptr;
  }
  void* ptr = &custom_pool[pool_index];
  pool_index += size;
  return ptr;
}

// Reverb preset changes clear the buffers, so the wet path is muted for
// ~21 ms (ramp 0 -> 1 at 0.001/sample) and the preset is loaded at 10%.
int reverb_mode = -1;
bool mode_changed = false;
bool trigger_mode = false;
float ramp = 1.0f;

void ApplyReverbMode() {
  reverb->ClearBuffers();
  switch (reverb_mode) {
    case Hothouse::TOGGLESWITCH_UP:
      reverb->initFactoryChorus();
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
      reverb->initFactoryRubiKaFields();
      break;
    default:
      reverb->initFactoryMediumSpace();
      break;
  }
  reverb->SetParameter(Parameter2::LineCount, 2);
  force_reset = true;
}

// --- Delay ---

constexpr size_t kMaxDelay = 48000 * 4;     // 4 s
constexpr size_t kMaxDelayRev = 48000 * 8;  // reverse needs double

DelayLineOct<float, kMaxDelay> DSY_SDRAM_BSS del_oct_l;
DelayLineOct<float, kMaxDelay> DSY_SDRAM_BSS del_oct_r;
DelayLineReverse<float, kMaxDelayRev> DSY_SDRAM_BSS del_rev_l;
DelayLineReverse<float, kMaxDelayRev> DSY_SDRAM_BSS del_rev_r;

struct NeptuneDelay {
  DelayLineOct<float, kMaxDelay>* del;
  DelayLineReverse<float, kMaxDelayRev>* del_rev;
  Tone lowpass;  // tames octave fizz; also darkens each repeat
  float current_delay = 2400.0f;
  float target_delay = 2400.0f;
  float feedback = 0.0f;
  bool active = true;
  bool reverse = false;

  float Process(float in) {
    fonepole(current_delay, target_delay, 0.0002f);
    del->SetDelay(current_delay);
    del_rev->SetDelay1(current_delay);

    const float read = lowpass.Process(del->Read());
    const float read_rev = del_rev->ReadRev();

    // Both lines always get written so mode switches are instant. The reverse
    // line takes the octave read as feedback, hence reverse-octave repeats.
    const float write = feedback * read + (active ? in : 0.0f);
    del->Write(write);
    del_rev->Write(write);

    return reverse ? read_rev : read;
  }
};

NeptuneDelay delay_l, delay_r;

// --- Controls ---

bool KnobMoved(float old_value, float new_value) {
  return fabsf(new_value - old_value) > 0.005f;
}

// Cheap, mostly energy-constant crossfade:
// https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
void CrossfadeGains(float x, float* to_gain, float* from_gain) {
  const float x2 = 1.0f - x;
  const float a = x * x2;
  const float b = a * (1.0f + 1.4186f * a);
  const float c = b + x;
  const float d = b + x2;
  *to_gain = c * c;
  *from_gain = d * d;
}

void UpdateControls() {
  // FOOTSWITCH_1 held = alternate functions on KNOBs 1, 2 and 4
  const bool alt_held = hw.switches[Hothouse::FOOTSWITCH_1].Pressed();
  if (alternate_mode && !alt_held) {
    force_reset = true;
  }
  alternate_mode = alt_held;

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  const int new_reverb_mode =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
  if (new_reverb_mode != reverb_mode) {
    reverb_mode = new_reverb_mode;
    mode_changed = true;
  }
  if (mode_changed && ramp > 0.95f) {  // rate-limit preset reloads
    ramp = 0.0f;
    trigger_mode = true;
  }

  const Hothouse::ToggleswitchPosition delay_mode =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
  const bool octave = delay_mode == Hothouse::TOGGLESWITCH_MIDDLE;
  const bool reverse = delay_mode == Hothouse::TOGGLESWITCH_DOWN;
  delay_l.del->setOctave(octave);
  delay_r.del->setOctave(octave);
  delay_l.reverse = delay_r.reverse = reverse;

  const float v_decay = p_decay.Process();
  const float v_mix = p_mix.Process();
  const float v_delay_time = p_delay_time.Process();
  const float v_mod = p_mod.Process();
  const float v_filter = p_filter.Process();
  const float v_feedback = p_feedback.Process();

  if (KnobMoved(p_mix_last, v_mix) || force_reset) {
    if (alternate_mode) {
      CrossfadeGains(v_mix, &delay_mix, &reverb_mix);
    } else {
      CrossfadeGains(v_mix, &wet_mix, &dry_mix);
    }
    p_mix_last = v_mix;
  }

  if (KnobMoved(p_filter_last, v_filter) || force_reset) {
    reverb->SetParameter(Parameter2::PostCutoffFrequency, v_filter);
    p_filter_last = v_filter;
  }

  // Delay is 0.05 to 4 s; under 1% of the knob stops new input to it
  delay_l.active = delay_r.active = v_delay_time > 0.01f;
  delay_l.target_delay = delay_r.target_delay =
      2400.0f + v_delay_time * 189600.0f;
  delay_l.feedback = delay_r.feedback = v_feedback;

  if (KnobMoved(p_decay_last, v_decay) || force_reset) {
    reverb->SetParameter(
        alternate_mode ? Parameter2::PreDelay : Parameter2::LineDecay, v_decay);
    p_decay_last = v_decay;
  }

  if (KnobMoved(p_mod_last, v_mod) || force_reset) {
    reverb->SetParameter(
        alternate_mode ? Parameter2::LineModRate : Parameter2::LineModAmount,
        v_mod);
    p_mod_last = v_mod;
  }

  force_reset = false;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();
  UpdateControls();

  const Hothouse::ToggleswitchPosition routing =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
  const float engage_target = bypass ? 0.0f : 1.0f;

  for (size_t i = 0; i < size; ++i) {
    fonepole(engage, engage_target, 0.004f);

    const float in_l = in[0][i];
    const float in_r = NEPTUNE_TRUE_STEREO ? in[1][i] : in_l;
    const float send_l = in_l * engage;
    const float send_r = in_r * engage;

    const float delay_out_l = delay_l.Process(send_l) * delay_mix;
    const float delay_out_r = delay_r.Process(send_r) * delay_mix;

    // UP and DOWN feed the delay into the reverb; MIDDLE runs them in parallel
    float rev_in_l = send_l;
    float rev_in_r = send_r;
    if (routing != Hothouse::TOGGLESWITCH_MIDDLE) {
      rev_in_l += delay_out_l;
      rev_in_r += delay_out_r;
    }

    float rev_out_l, rev_out_r;
    reverb->Process(&rev_in_l, &rev_in_r, &rev_out_l, &rev_out_r, 1);

    // DOWN (series) leaves the dry delay out of the output
    float wet_l = rev_out_l * reverb_mix * 1.2f;
    float wet_r = rev_out_r * reverb_mix * 1.2f;
    if (routing != Hothouse::TOGGLESWITCH_DOWN) {
      wet_l += delay_out_l;
      wet_r += delay_out_r;
    }

    wet_l *= wet_mix;
    wet_r *= wet_mix;

    if (ramp < 1.0f) {
      ramp += 0.001f;
      wet_l = wet_r = 0.0f;
      if (ramp > 0.1f && trigger_mode) {
        trigger_mode = false;
        ApplyReverbMode();
        mode_changed = false;
      }
    }

    // Bypassed, dry rides to unity and the trails sit on top
    const float dry_gain = dry_mix + (1.0f - dry_mix) * (1.0f - engage);
    out[0][i] = in_l * dry_gain + wet_l * wet_mix;
    out[1][i] = in_r * dry_gain + wet_r * wet_mix;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  const float sample_rate = hw.AudioSampleRate();

  AudioLib::ValueTables::Init();
  CloudSeed::FastSin::Init();

  reverb = new CloudSeed::ReverbController(static_cast<int>(sample_rate));
  reverb->SetParameter(Parameter2::LineCount, 2);
  reverb->SetParameter(Parameter2::PreDelay, 0.0f);

  del_oct_l.Init();
  del_oct_r.Init();
  del_rev_l.Init();
  del_rev_r.Init();
  delay_l.del = &del_oct_l;
  delay_l.del_rev = &del_rev_l;
  delay_r.del = &del_oct_r;
  delay_r.del_rev = &del_rev_r;
  delay_l.lowpass.Init(sample_rate);
  delay_l.lowpass.SetFreq(3000.0f);
  delay_r.lowpass.Init(sample_rate);
  delay_r.lowpass.SetFreq(3000.0f);

  using daisy::Parameter;
  p_decay.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
  p_mix.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
  p_delay_time.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  p_mod.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  p_filter.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::CUBE);
  p_feedback.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  led_alt.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    led_alt.Set(alternate_mode ? 1.0f : 0.0f);
    led_alt.Update();
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();

    hw.CheckResetToBootloader();
  }
  return 0;
}
