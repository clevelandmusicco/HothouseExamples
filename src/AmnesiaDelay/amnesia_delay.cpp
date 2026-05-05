// Amnesia Delay for Cleveland Music Co. Hothouse DIY DSP Platform
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
// A simulated BBD-style delay loosely modeled on the Electro-Harmonix Deluxe
// Memory Man.
//
// Controls (mono, single-engine):
//   KNOB 1 - BLEND       Equal-power dry/wet crossfade. Noon = equal levels.
//   KNOB 2 - FEEDBACK    Number/level of repeats. High settings self-oscillate.
//   KNOB 3 - DELAY       Delay time, ~30 ms to ~550 ms (logarithmic taper).
//   KNOB 4 - DEPTH       Modulation depth (delay-time wobble).
//   KNOB 5 - RATE        Modulation rate. CCW = chorus (slow), CW = vibrato.
//   KNOB 6 - CLOCK NOISE Amount of BBD clock leakage and bias-drift noise.
//   FOOTSWITCH 2         Engage / bypass the effect.
//   LED 2                Lit when the effect is engaged.
// -----------------------------------------------------------------------------

#include <cmath>

#include "daisysp.h"
#include "hothouse.h"

// Maximum delay length in samples. The DMM tops out at ~550 ms; we allocate
// 600 ms of headroom so the modulation LFO never reads past the end of the
// buffer at the longest delay setting.
constexpr float kMaxDelaySeconds = 0.6f;
constexpr size_t kMaxDelaySamples =
    static_cast<size_t>(48000.0f * kMaxDelaySeconds);

// MN3005 BBD chip stage count. Used for modelling the audible clock frequency.
constexpr float kBbdStages = 4096.0f;

// Two-pi as float; using a literal avoids any namespace surprises with M_PI.
constexpr float kTwoPi = 6.28318530717958647692f;

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisy::System;
using daisysp::DelayLine;
using daisysp::fclamp;
using daisysp::fonepole;
using daisysp::WhiteNoise;

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

Hothouse hw;

// The delay buffer must live in the Daisy Seed's external SDRAM, not on-chip
// RAM. The DSY_SDRAM_BSS attribute tells the linker to place it there.
DelayLine<float, kMaxDelaySamples> DSY_SDRAM_BSS delay_line;

// daisy::Parameter wraps an analog control with range/taper conversion.
Parameter p_blend;
Parameter p_feedback;
Parameter p_delay;
Parameter p_depth;
Parameter p_rate;
Parameter p_clock;

// One-pole low-pass filter cascade used to darken the wet signal in a
// BBD-like way. Each stage gives ~6 dB/oct, so cascading kBbdLpfStages of
// them yields a steeper rolloff (roughly 6*N dB/oct in the stopband). Real
// BBD pedals use 4th-to-6th-order elliptic anti-alias filters around 3 kHz;
// we approximate that character at a fraction of the CPU cost.
//
// We roll our own one-pole rather than using daisysp::Tone, because Tone
// lives in DaisySP-LGPL. It's also a useful thing for a learner to see
// written out: a single coefficient `a` and a single sample of state `z`,
// with transfer function
//
//     H(z) = (1 - a) / (1 - a * z^-1)
//
// where a = exp(-2π fc / fs) places the -3 dB point at fc Hz. As a → 1 the
// filter clamps shut (DC only); as a → 0 it passes everything.
struct OnePoleLpf {
  float a = 0.0f;  // pole coefficient
  float z = 0.0f;  // y[n-1]

  void SetCutoff(float fc_hz, float fs_hz) {
    a = expf(-kTwoPi * fc_hz / fs_hz);
  }

  float Process(float x) {
    z = (1.0f - a) * x + a * z;
    return z;
  }

  void Reset() { z = 0.0f; }
};

constexpr int kBbdLpfStages = 4;
OnePoleLpf bbd_lpf[kBbdLpfStages];

// Noise generator and a low-pass to colour the "bias drift" hiss component
// of the clock-noise model.
WhiteNoise hiss_noise;
OnePoleLpf hiss_lpf;

// LFO and clock phase accumulators. We never reset these to zero; we only
// subtract whole multiples of 2*pi to keep them in a sensible range. sinf()
// is exactly periodic, so this introduces no discontinuity, no dead zone,
// and no sawtooth-like reset artefact.
float lfo_phase = 0.0f;
float clock_phase = 0.0f;

// Smoothed parameter values (updated every sample inside the AudioCallback).
// Smoothing prevents zipper noise as a knob is turned, and is essential on
// the DELAY knob to avoid clicks from large jumps in the read tap.
struct Smoothed {
  float current;
  float target;
  float coeff;  // smaller = slower; see daisysp::fonepole.

  void Tick() { fonepole(current, target, coeff); }
};

// Coefficients chosen empirically: very slow on DELAY (audible glides on big
// changes are part of the DMM's sound), faster on the rest.
Smoothed s_blend{0.5f, 0.5f, 0.0008f};
Smoothed s_feedback{0.3f, 0.3f, 0.0008f};
Smoothed s_delay{4800.0f, 4800.0f, 0.0001f};  // samples
Smoothed s_depth{0.0f, 0.0f, 0.0008f};
Smoothed s_rate{0.5f, 0.5f, 0.0008f};         // Hz
Smoothed s_clock{0.0f, 0.0f, 0.0008f};

// Sample rate, populated in main() after hardware init.
float sample_rate = 48000.0f;

// Bypass / LED state.
Led led_bypass;
bool bypass = true;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

// Equal-power crossfade gains. Maps x in [0, 1] to a (dry, wet) pair lying on
// the unit circle (cos^2 + sin^2 = 1), so total POWER stays constant across
// the sweep. A linear crossfade would dip ~3 dB at noon; this does not.
static inline void EqualPowerGains(float x, float* dry_gain, float* wet_gain) {
  const float angle = x * (kTwoPi * 0.25f); 
  *dry_gain = cosf(angle);
  *wet_gain = sinf(angle);
}

// Cheap soft clipper used in the feedback path. tanh-like shape keeps
// runaway oscillation musical instead of letting it explode to NaN, while
// still allowing strong self-oscillation when FEEDBACK is cranked. This
// stands in for the NE570/571 compander's gentle limiting in the real DMM.
static inline float SoftClip(float x) { return tanhf(x); }

// Advance a phase accumulator by `inc` radians, wrapping to [0, 2*pi] by
// integer-period subtraction. Wrapping this way (rather than reset-to-zero
// or modulo with phase = fmodf(...)) preserves continuity exactly: sinf
// is periodic, so f(x) == f(x - 2*pi) for any x. No dead zone, no kink.
static inline void AdvancePhase(float* phase, float inc) {
  *phase += inc;
  if (*phase >= kTwoPi) {
    *phase -= kTwoPi;
  }
}

// -----------------------------------------------------------------------------
// Audio callback
// -----------------------------------------------------------------------------

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  // Read all hardware controls once per audio block.
  hw.ProcessAllControls();

  // Pull fresh targets from the knobs. Smoothing happens per-sample below.
  s_blend.target = p_blend.Process();
  s_feedback.target = p_feedback.Process();
  s_delay.target = p_delay.Process();
  s_depth.target = p_depth.Process();
  s_rate.target = p_rate.Process();
  s_clock.target = p_clock.Process();

  // Toggle bypass on rising edge of FOOTSWITCH 2. This is the canonical
  // Hothouse pattern.
  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    const float dry = in[0][i];

    // Bypass path: just pass the dry signal through, untouched. We still
    // duplicate to both output channels so users with stereo cables hear
    // signal on both sides (the rest of the effect is mono throughout).
    if (bypass) {
      out[0][i] = out[1][i] = dry;
      continue;
    }

    // Smooth knob movements one sample at a time.
    s_blend.Tick();
    s_feedback.Tick();
    s_delay.Tick();
    s_depth.Tick();
    s_rate.Tick();
    s_clock.Tick();

    // -------------------------------------------------------------------
    // 1. LFO  (smooth sine, no wavetable, no reset)
    // -------------------------------------------------------------------
    // The phase accumulator advances continuously and is fed directly to
    // sinf() each sample. No table lookup, no quantisation, no sawtooth
    // shape mucking about under the modulation.
    AdvancePhase(&lfo_phase, kTwoPi * s_rate.current / sample_rate);
    const float lfo = sinf(lfo_phase);

    // -------------------------------------------------------------------
    // 2. Modulated read tap
    // -------------------------------------------------------------------
    // Center delay (in samples) plus an LFO-driven offset. Cap the
    // modulation amplitude so the read pointer can never go negative or
    // off the end of the buffer.
    //
    // ~5 ms (240 samples at 48 kHz) of swing is plenty for both gentle
    // chorus and clear vibrato when paired with the rate range below.
    constexpr float kMaxModSamples = 240.0f;
    const float mod_samples = s_depth.current * kMaxModSamples;
    float read_samples = s_delay.current + mod_samples * lfo;
    read_samples = fclamp(read_samples, 1.0f,
                          static_cast<float>(kMaxDelaySamples) - 2.0f);

    delay_line.SetDelay(read_samples);
    float wet = delay_line.Read();

    // -------------------------------------------------------------------
    // 3. BBD darkening (anti-alias / capacitor degradation)
    // -------------------------------------------------------------------
    // Real BBDs lose high frequencies on every charge transfer; longer
    // delays mean slower clocks, lower Nyquist, and even darker tone.
    // We approximate this by tying the cutoff to the delay setting.
    //
    // 8 kHz at the shortest delay to ~1.6 kHz at the longest. Cascading
    // four 1-pole sections produces the lush, dark repeats the DMM is
    // famous for (and helpfully tames any feedback runaway too).
    const float darkening_ratio = s_delay.current / kMaxDelaySamples;
    const float bbd_cutoff = fclamp(8000.0f - 6400.0f * darkening_ratio,
                                    1500.0f, 8000.0f);
    for (int s = 0; s < kBbdLpfStages; ++s) {
      bbd_lpf[s].SetCutoff(bbd_cutoff, sample_rate);
      wet = bbd_lpf[s].Process(wet);
    }

    // -------------------------------------------------------------------
    // 4. BBD clock-noise model
    // -------------------------------------------------------------------
    // The MN3005's clock runs at f_c = stages / (2 * delay_seconds).
    // At max delay (~550 ms) this lands around 3.7 kHz - audible whine.
    // At min delay (~30 ms) it sits well above hearing, but folds back
    // through aliasing. We let it alias naturally by letting the phase
    // wrap past Nyquist; this is an unconventional bit of DSP here ...
    // we are deliberately NOT band-limiting the clock tone, because the
    // aliasing *IS* the artifact we want to hear.
    const float delay_sec = s_delay.current / sample_rate;
    const float clock_hz = kBbdStages / (2.0f * delay_sec);
    AdvancePhase(&clock_phase, kTwoPi * clock_hz / sample_rate);
    const float clock_tone = sinf(clock_phase);

    // Bias-drift hiss: filtered white noise scaled by both the CLOCK NOISE
    // knob and the delay length. Longer delays = more capacitor stages
    // contributing noise = louder hiss, mirroring the real chip.
    const float hiss = hiss_lpf.Process(hiss_noise.Process());

    // Mix the two noise components in. The 0.0035/0.001 weights are taste:
    // the tone should peek out audibly with the knob fully CW.
    const float clock_amount = s_clock.current;
    wet += clock_tone * clock_amount * 0.0035f;
    wet += hiss * clock_amount * darkening_ratio * 0.001f;

    // -------------------------------------------------------------------
    // 5. Feedback write
    // -------------------------------------------------------------------
    // Soft-clip the signal heading back into the delay line. This lets
    // the user crank FEEDBACK past unity for runaway oscillation while
    // keeping the buffer values bounded (no NaNs, no DC explosion). The
    // soft saturation also adds a touch of warmth, just as the real
    // DMM's compander does on its way around the loop.
    const float into_delay = SoftClip(dry + wet * s_feedback.current);
    delay_line.Write(into_delay);

    // -------------------------------------------------------------------
    // 6. Equal-power dry/wet blend
    // -------------------------------------------------------------------
    float dry_gain;
    float wet_gain;
    EqualPowerGains(s_blend.current, &dry_gain, &wet_gain);
    const float mixed = dry * dry_gain + wet * wet_gain;

    // Mono effect; write the same sample to both output channels.
    out[0][i] = out[1][i] = mixed;
  }
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // small block size keeps modulation latency low
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sample_rate = hw.AudioSampleRate();

  delay_line.Init();

  // Knob mapping. The Parameter helper handles ADC to value scaling and taper.
  p_blend.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);

  // Allow feedback slightly past unity so users can drive the delay into
  // self-oscillation, just like the original.
  p_feedback.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.05f, Parameter::LINEAR);

  // ~30 ms to ~550 ms, log taper so short delays have more knob travel
  // (consistent with how the original DMM's pot is laid out).
  p_delay.Init(hw.knobs[Hothouse::KNOB_3], sample_rate * 0.030f,
               sample_rate * 0.550f, Parameter::LOGARITHMIC);

  p_depth.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);

  // 0.3 Hz (slow chorus shimmer) to 7 Hz (full vibrato wobble).
  // Logarithmic taper gives finer control over the slow chorus end.
  // A bit more range than a DMM (~0.75 to ~5 Hz); adjust to taste.
  p_rate.Init(hw.knobs[Hothouse::KNOB_5], 0.3f, 7.0f, Parameter::LOGARITHMIC);

  p_clock.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  // The BBD darkening filter cascade is default-constructed and gets its
  // cutoff set every sample inside the audio callback. Nothing to do here.

  // Hiss filter cuts the harshness of pure white noise so the bias-drift
  // effect sits more like real BBD hiss than digital snow.
  hiss_noise.Init();
  hiss_lpf.SetCutoff(3000.0f, sample_rate);

  // LED 2 indicates whether the effect is engaged.
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    System::Delay(10);

    // Standard Hothouse idiom: hold left footswitch 2s for DFU mode.
    hw.CheckResetToBootloader();
  }
  return 0;
}
