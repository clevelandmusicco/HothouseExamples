// LibreVerb for Cleveland Music Co. Hothouse DIY DSP Platform
// Copyright (C) 2026 Ricky Sheaves <ricky@clevelandmusicco.com>
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
//
// -----------------------------------------------------------------------------
// LibreVerb is a mono-to-stereo reverb based on Freeverb, with some ear candy:
//   - Pre-delay
//   - Highs damping (lowpass) in the comb feedback path
//   - Lows damping (highpass) in the comb feedback path
//   - Modulated comb read taps (Lexicon-style chorused tail)
//   - Plate / Hall / Cathedral "size" presets
//   - Mono / Stereo / Wide stereo modes
//
// References:
//   Freeverb original by Jezar at Dreampoint:
//     https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
//   Schroeder/Moorer reverb structure:
//     https://en.wikipedia.org/wiki/Reverberation#Algorithmic_reverb
//   Mid/side processing (used for the Wide mode):
//     https://en.wikipedia.org/wiki/Joint_(audio_engineering)#M/S_stereo
// -----------------------------------------------------------------------------

#include <cmath>

#include "daisysp.h"
#include "freeverb_core.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisysp::DcBlock;
using daisysp::DelayLine;
using daisysp::fclamp;
using daisysp::fonepole;
using libreverb::Allpass;
using libreverb::Comb;
using libreverb::kApLensL;
using libreverb::kCombLensL;
using libreverb::kNumAllpasses;
using libreverb::kNumCombs;

constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kHalfPi = 1.57079632679489661923f;
constexpr float kSrRef = 44100.0f;

// Pre-delay buffer. 250ms at 48kHz = 12000 samples. Round up to 13000 so the
// SetDelay(float) fractional tap can never read past the end.
constexpr size_t kPreDelayBufSize = 13000;

// SDRAM buffers. The comb / allpass classes wrap pow2-sized buffers so they
// can use bit-mask wrapping instead of integer modulo. See freeverb_core.h.
float DSY_SDRAM_BSS g_comb_buf_L[kNumCombs][libreverb::kCombBufSize];
float DSY_SDRAM_BSS g_comb_buf_R[kNumCombs][libreverb::kCombBufSize];
float DSY_SDRAM_BSS g_ap_buf_L[kNumAllpasses][libreverb::kApBufSize];
float DSY_SDRAM_BSS g_ap_buf_R[kNumAllpasses][libreverb::kApBufSize];
DelayLine<float, kPreDelayBufSize> DSY_SDRAM_BSS g_pre_delay;

// --- Globals ---

Hothouse hw;
Led led_bypass;
bool bypass = true;

float sample_rate = 48000.0f;
float sr_scale = 48000.0f / kSrRef;

Comb comb_L[kNumCombs];
Comb comb_R[kNumCombs];
Allpass ap_L[kNumAllpasses];
Allpass ap_R[kNumAllpasses];
DcBlock dc_in;

Parameter p_mix, p_decay, p_predelay, p_highs, p_lows, p_mod;

// LFO state. Per-channel phase so L and R can move independently.
// Each comb in a channel uses the same channel phase plus a fixed per-comb
// offset, which decorrelates the modulation across the 8-comb bank.
float lfo_phase_L = 0.0f;
float lfo_phase_R = kHalfPi;
float comb_phase_offset[kNumCombs];

// --- Tables ---

// Toggle position arrays: index 0 = UP, 1 = MIDDLE, 2 = DOWN.

// Size mode scalers applied to the comb / allpass delay lengths.
constexpr float kModeScale[3] = {0.6f, 1.0f, 1.4f};  // Plate / Hall / Cathedral

// Modulation rate in Hz. UP = slow Lexicon-style drift, MIDDLE = Strymon
// Blue Sky territory, DOWN = fast warble.
constexpr float kModSpeedHz[3] = {0.15f, 0.5f, 1.6f};

// Stereo-width modes (toggle 1). The wet bus is recombined as mid/side:
//   wet_L = mid_gain*M + side_gain*S
//   wet_R = mid_gain*M - side_gain*S
// where M = 0.5*(L+R) and S = 0.5*(L-R).
//   UP     = MONO   : side killed, mid boosted by sqrt(2) so collapsing the
//                     two decorrelated tails to one channel keeps the level.
//   MIDDLE = STEREO : mid_gain = side_gain = 1, which is just the plain L/R
//                     pair again (the identity case).
//   DOWN   = WIDE   : side pushed up, mid pulled down a touch. The 1.4/0.8
//                     ratio (~1.75x more side) is a clearly wider field; the
//                     mid trim keeps the loudness bump down to about +1 dB.
//                     Push side_gain much past ~1.5 and the centre starts to
//                     sound phasey.
constexpr float kMidGain[3] = {1.41421356f, 1.0f, 0.8f};
constexpr float kSideGain[3] = {0.0f, 1.0f, 1.4f};

// The 8 parallel combs sum to a hot bus. This trim brings 100%-wet roughly
// level with dry signal amplitude.
constexpr float kWetTrim = 0.12f;

// --- Smoothed parameter helper ---

struct Smoothed {
  float current;
  float target;
  float coeff;  // smaller = slower glide
  inline void Tick() { fonepole(current, target, coeff); }
};

// Slow on pre-delay (audible pitch artifact otherwise) and the size mode
// scale (we WANT the glide on mode changes -- it sounds cool like a room
// morphing). Everything else gets faster smoothing to keep knob travel
// responsive without zipper noise. The width gains glide too, just fast
// enough that flicking toggle 1 doesn't pop.
Smoothed s_mix{0.5f, 0.5f, 0.0008f};
Smoothed s_decay{0.85f, 0.85f, 0.0008f};
Smoothed s_predelay{0.0f, 0.0f, 0.00015f};  // samples
Smoothed s_highs{0.4f, 0.4f, 0.0008f};
Smoothed s_lows{0.0f, 0.0f, 0.0008f};
Smoothed s_mod{0.0f, 0.0f, 0.0008f};
Smoothed s_mode_scale{1.0f, 1.0f, 0.00012f};
Smoothed s_mid{1.0f, 1.0f, 0.0008f};
Smoothed s_side{1.0f, 1.0f, 0.0008f};

// --- Helpers ---

// Equal-power dry/wet gains: a linear crossfade dips ~3 dB in the middle,
// this one stays flat. cos^2 + sin^2 = 1, so the total power is constant.
// A cubic taper shifts the 50/50 crossover to 3 o'clock (75% of rotation),
// giving more room for wet adjustments in the lower sweep. Derived from
// f(x) = x^2*(ax+b) with constraints f(0)=0, f(0.75)=0.5, f(1)=1 ->
// a=4/9, b=5/9.
static inline void EqualPowerGains(float mix, float* dry_g, float* wet_g) {
  const float shaped = mix * mix * (mix * (4.0f / 9.0f) + (5.0f / 9.0f));
  const float a = shaped * kHalfPi;
  *dry_g = cosf(a);
  *wet_g = sinf(a);
}

// Advance an LFO phase by `inc` radians, wrapping to [0, 2*pi) by
// subtraction (preserves continuity of sinf).
static inline void AdvancePhase(float* phase, float inc) {
  *phase += inc;
  if (*phase >= kTwoPi) *phase -= kTwoPi;
}

// --- Audio callback ---

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Pull fresh targets from the knobs (smoothing happens per-sample below).
  s_mix.target = p_mix.Process();
  s_decay.target = p_decay.Process();
  s_predelay.target = p_predelay.Process();
  s_highs.target = p_highs.Process();
  s_lows.target = p_lows.Process();
  s_mod.target = p_mod.Process();

  // FOOTSWITCH 2 = bypass (canonical Hothouse pattern).
  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // Toggle positions.
  const int sw1 =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);  // width
  const int sw2 = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);  // size
  const int sw3 =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);  // mod speed

  s_mode_scale.target = kModeScale[sw2];
  s_mid.target = kMidGain[sw1];
  s_side.target = kSideGain[sw1];

  const float lfo_inc = kTwoPi * kModSpeedHz[sw3] / sample_rate;

  for (size_t i = 0; i < size; ++i) {
    const float dry = in[0][i];

    if (bypass) {
      out[0][i] = out[1][i] = dry;
      continue;
    }

    // --- Smooth per sample ---
    s_mix.Tick();
    s_decay.Tick();
    s_predelay.Tick();
    s_highs.Tick();
    s_lows.Tick();
    s_mod.Tick();
    s_mode_scale.Tick();
    s_mid.Tick();
    s_side.Tick();

    // --- Per-sample derived params ---
    // Total delay length scaler combines SR ratio (Daisy is 48k, Freeverb
    // tunings are 44.1k) and the current SIZE mode (Plate/Hall/Cathedral).
    const float total_scale = sr_scale * s_mode_scale.current;

    // Decay -> feedback. We cap at 0.999 (about 30s RT60 on the longest
    // comb at Hall size), so even at max DECAY the algorithm stays bounded.
    const float fb = 0.7f + 0.299f * s_decay.current;

    // Highs damp -> comb LPF coefficient. 0 = no damping (bright tail);
    // 0.95 = very dark, like a heavily-curtained hall.
    const float damp = s_highs.current * 0.95f;

    // Lows damp -> comb HPF coefficient. We keep a small floor so a long
    // decay with LOWS at zero can never ring up DC or sub-bass mud. With
    // the floor in place the tail is bulletproof; without it the user can
    // get into runaway territory pretty fast at max DECAY.
    constexpr float kHpFloor = 0.0026f;  // ~20 Hz cut
    constexpr float kHpMax = 0.052f;     // ~400 Hz cut
    const float hp_a = kHpFloor + s_lows.current * (kHpMax - kHpFloor);

    // Modulation depth in samples. ~16 samples peak-to-peak is plenty of
    // movement -- more gets warbly and starts to step out of "chorused
    // reverb" into "detuned wonkery".
    const float mod_depth_samples = s_mod.current * 16.0f;

    AdvancePhase(&lfo_phase_L, lfo_inc);
    AdvancePhase(&lfo_phase_R, lfo_inc);

    // Push per-comb config. Cheap (8 combs x small struct setters), and
    // doing it per-sample keeps mode glides and decay smoothing audible.
    for (int c = 0; c < kNumCombs; ++c) {
      const float base_L = static_cast<float>(kCombLensL[c]) * total_scale;
      const float base_R =
          static_cast<float>(kCombLensL[c] + libreverb::kStereoSpread) *
          total_scale;
      comb_L[c].SetBaseDelay(base_L);
      comb_R[c].SetBaseDelay(base_R);
      comb_L[c].SetFeedback(fb);
      comb_R[c].SetFeedback(fb);
      comb_L[c].SetDamp(damp);
      comb_R[c].SetDamp(damp);
      comb_L[c].SetLowDamp(hp_a);
      comb_R[c].SetLowDamp(hp_a);
      const float ph = comb_phase_offset[c];
      comb_L[c].SetModOffset(mod_depth_samples * sinf(lfo_phase_L + ph));
      comb_R[c].SetModOffset(mod_depth_samples * sinf(lfo_phase_R + ph));
    }
    for (int a = 0; a < kNumAllpasses; ++a) {
      ap_L[a].SetDelay(static_cast<int>(kApLensL[a] * total_scale));
      ap_R[a].SetDelay(static_cast<int>(
          (kApLensL[a] + libreverb::kStereoSpread) * total_scale));
    }

    // --- 1. Pre-delay (mono) ---
    // DC-block the input before it goes into the buffer; otherwise DC in
    // the input bias can ring up in the comb tail at long decay times.
    g_pre_delay.SetDelay(fclamp(s_predelay.current, 1.0f,
                                static_cast<float>(kPreDelayBufSize) - 2.0f));
    const float dc_blocked = dc_in.Process(dry);
    g_pre_delay.Write(dc_blocked);
    const float predelayed = g_pre_delay.Read();

    // --- 2. Parallel combs into per-channel wet buses ---
    // Both banks see the same (mono) input; the L/R delay-length asymmetry
    // is what decorrelates the two tails.
    float bus_L = 0.0f;
    float bus_R = 0.0f;
    for (int c = 0; c < kNumCombs; ++c) {
      bus_L += comb_L[c].Process(predelayed);
      bus_R += comb_R[c].Process(predelayed);
    }

    // --- 3. Series allpasses for diffusion ---
    for (int a = 0; a < kNumAllpasses; ++a) {
      bus_L = ap_L[a].Process(bus_L);
      bus_R = ap_R[a].Process(bus_R);
    }

    // --- 4. Stereo width (mid/side recombine) ---
    // With mid_gain = side_gain = 1 this is the plain L/R pair. Other gains
    // squash it to mono or stretch it wide; see kMidGain / kSideGain above.
    const float mid = 0.5f * (bus_L + bus_R);
    const float side = 0.5f * (bus_L - bus_R);
    const float wet_L = mid * s_mid.current + side * s_side.current;
    const float wet_R = mid * s_mid.current - side * s_side.current;

    // --- 5. Equal-power dry/wet mix ---
    float dry_g;
    float wet_g;
    EqualPowerGains(s_mix.current, &dry_g, &wet_g);
    out[0][i] = dry * dry_g + wet_L * wet_g * kWetTrim;
    out[1][i] = dry * dry_g + wet_R * wet_g * kWetTrim;
  }
}

// --- Main ---

int main() {
  hw.Init();
  // Small block = low control/modulation latency and no audible block-rate
  // whine (~1 kHz at block 48).
  // TODO: block size of 4 produces "frying eggs" artifacts. But why? We
  // should have plenty of CPU headroom ...
  hw.SetAudioBlockSize(8);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sample_rate = hw.AudioSampleRate();
  sr_scale = sample_rate / kSrRef;

  // Init combs / allpasses and compute per-comb modulation phase offsets.
  for (int c = 0; c < kNumCombs; ++c) {
    comb_L[c].Init(g_comb_buf_L[c]);
    comb_R[c].Init(g_comb_buf_R[c]);
    comb_phase_offset[c] = static_cast<float>(c) * (kTwoPi / kNumCombs);
  }
  for (int a = 0; a < kNumAllpasses; ++a) {
    ap_L[a].Init(g_ap_buf_L[a]);
    ap_R[a].Init(g_ap_buf_R[a]);
  }
  g_pre_delay.Init();

  dc_in.Init(sample_rate);

  p_mix.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
  p_decay.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
  // Pre-delay: 1 sample to 250 ms (in samples). Log taper so most of the
  // travel is on short pre-delays, where small changes are most audible.
  p_predelay.Init(hw.knobs[Hothouse::KNOB_3], 1.0f, sample_rate * 0.250f,
                  Parameter::LOGARITHMIC);
  p_highs.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  p_lows.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  p_mod.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();

    // Hold FOOTSWITCH 1 for 2 seconds to enter DFU/flashing mode.
    hw.CheckResetToBootloader();
  }
  return 0;
}
