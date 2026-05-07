// EchoKing for Cleveland Music Co. Hothouse DIY DSP Platform
// Digital model of the Maestro Echoplex family: EP-1, EP-2, and EP-3
// Copyright (C) 2026 Cleveland Music Co. <code@clevelandmusicco.com>
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

// The Maestro Echoplex was a tape loop echo unit first produced around 1959.
// The EP-1 and EP-2 used tube preamps (12AX7); the EP-3 switched to solid
// state. Three things define the sound: the preamp (always in the signal path,
// coloring the dry tone), the tape medium (bandwidth limits, soft saturation,
// and repeats that darken each pass), and the transport mechanics (wow,
// flutter, and a characteristic pitch glide when delay time changes). All three
// are modeled here.
//
// Each model is a constexpr profile struct. Switching models just changes which
// profile the engine reads -- no model-specific branches in the hot loop.
//
// See: https://en.wikipedia.org/wiki/Maestro_Echoplex

#include <cmath>

#include "daisysp.h"
#include "hothouse.h"

// 800 ms of headroom covers the long delay range plus wow/flutter excursion.
// 800ms * 48kHz * 4 bytes/sample = ~150 KB, trivial for the 64 MB Daisy SDRAM.
constexpr float kMaxDelaySeconds = 0.8f;
constexpr size_t kMaxDelaySamples =
    static_cast<size_t>(48000.0f * kMaxDelaySeconds);

constexpr float kTwoPi = 6.28318530717958647692f;

// Flutter depth coefficient. This scalar sets how much delay-time modulation
// the flutter LFO produces relative to the current delay length. Try the lower
// values first and work your way up. Yea, this is OTT nerdy.
//
// Rule of thumb: flutter should add grit and instability without pitching
// every note into a wobble. If you can actively hear the flutter as vibrato
// rather than just "something isn't quite perfectly stable," it's too high.
// See also: flutter_scale in each model profile for per-model tuning.
//
// constexpr float kFlutterDepthCoeff = 0.0f;     // no flutter
// constexpr float kFlutterDepthCoeff = 0.0003f;  // barely audible
constexpr float kFlutterDepthCoeff = 0.0005f;  // start here
// constexpr float kFlutterDepthCoeff = 0.0008f;  // noticeable
// constexpr float kFlutterDepthCoeff = 0.0012f;  // old tape
// constexpr float kFlutterDepthCoeff = 0.0015f;  // pronounced
// constexpr float kFlutterDepthCoeff = 0.002f;   // heavy flutter

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

// --- Model profiles ---

// Everything that differs between EP-1, EP-2, and EP-3 lives in this struct.
// Switching the model toggle just swaps which profile the engine points at.
struct ModelProfile {
  float preamp_drive;      // input gain into the waveshaper
  float preamp_asymmetry;  // 0 = symmetric SS; > 0.05 = tube (even harmonics)
  float record_fc;         // record-path LPF cutoff, Hz (tape/head bandwidth)
  float playback_fc;    // playback-path LPF cutoff, Hz (additional head loss)
  float feedback_fc;    // feedback-path LPF cutoff, Hz (darkens each repeat)
  float wow_scale;      // multiplier on user wow/flutter depth (KNOB_6)
  float wow_rate;       // wow LFO base rate, Hz
  float flutter_scale;  // flutter depth multiplier (independent of wow)
  float flutter_rate;   // flutter LFO rate, Hz
  float noise_floor;    // tape hiss amplitude (linear scale on white noise)
  float max_feedback;   // feedback ceiling fed into the saturator
  float delay_smooth;   // one-pole coeff for delay-time glide; smaller = slower
  bool sos_capable;     // SOS mode available for this model
};

// EP-1: tube (12AX7 single-ended), early cartridge transport. Loosest
// mechanics, strongest even-harmonic coloration, most wow and flutter.
// The "vintage instability" model. No SOS -- that came later.
constexpr ModelProfile kEP1 = {
    1.4f,      // preamp_drive
    0.22f,     // preamp_asymmetry: single-ended triode, strong 2nd harmonic
    5500.0f,   // record_fc: slow tape formulation, less precise bias = soft HF
    4500.0f,   // playback_fc: early head gap loss adds more rolloff at playback
    3500.0f,   // feedback_fc: repeats go quite dark on each pass
    1.40f,     // wow_scale: imprecise AC motor = deep, slow pitch wander
    1.2f,      // wow_rate, Hz
    1.20f,     // flutter_scale
    8.0f,      // flutter_rate, Hz
    0.0006f,   // noise_floor: early Ampex oxide, notably hissy
    0.95f,     // max_feedback: approaches but doesn't quite reach unity
    0.00006f,  // delay_smooth: slow, heavy tape head -- long pitch glide
    false,     // sos_capable
};

// EP-2: tube (12AX7 + 12AU7), revised circuit. The canonical Echoplex sound.
// Thick, warm, the dry tone is perceptibly colored even without any echo.
// Sound-on-sound was available on later EP-2 units.
constexpr ModelProfile kEP2 = {
    1.5f,     // preamp_drive: slightly more drive for the "thick" EP-2 feel
    0.18f,    // preamp_asymmetry
    6000.0f,  // record_fc
    5000.0f,  // playback_fc
    4000.0f,  // feedback_fc
    1.00f,    // wow_scale: reference
    1.3f,     // wow_rate, Hz
    1.00f,    // flutter_scale: reference
    9.0f,     // flutter_rate, Hz
    0.0004f,  // noise_floor
    1.00f,    // max_feedback: can park right at unity for musical loop behavior
    0.00008f,  // delay_smooth
    true,      // sos_capable
};

// EP-3: solid state (JFET/FET), mid-1970s. Tighter, brighter, more focused.
// The "EP-3 preamp boost" is famous as a standalone tone shaper even without
// any echo dialed in. Better motor regulation means much less wow and flutter.
// Explicit Sound-on-Sound feature.
constexpr ModelProfile kEP3 = {
    1.2f,      // preamp_drive: SS clips less softly but with a "grippier" feel
    0.04f,     // preamp_asymmetry: near-symmetric (< 0.05 threshold ->
               // SSWaveshape)
    7500.0f,   // record_fc: solid-state record amp captures more high end
    6000.0f,   // playback_fc
    5000.0f,   // feedback_fc: repeats stay comparatively bright
    0.65f,     // wow_scale: regulated motor speed = much less pitch wander
    1.5f,      // wow_rate, Hz
    0.60f,     // flutter_scale
    10.5f,     // flutter_rate, Hz
    0.0002f,   // noise_floor: lower floor (no tube heater current, better SS
               // design)
    1.05f,     // max_feedback: slightly past unity for lively runaway
    0.00012f,  // delay_smooth: precision motor, crisper pitch response on time
               // change
    true,      // sos_capable
};

// --- Delay buffer ---

// DSY_SDRAM_BSS tells the linker to place this in the external SDRAM rather
// than on-chip SRAM. Without it, the 150 KB buffer would overflow on-chip RAM
// immediately and the device would not boot. Don't forget this attribute when
// declaring large delay buffers.
DelayLine<float, kMaxDelaySamples> DSY_SDRAM_BSS delay_line;

// --- One-pole LPF ---

// Simple IIR low-pass: one multiply, one add per sample. Used everywhere in
// the signal chain. y[n] = (1-a)*x[n] + a*y[n-1]. a near 1 = heavy rolloff.
// See: https://ccrma.stanford.edu/~jos/filters/One_Pole.html
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
};

// --- Globals ---

Hothouse hw;

// Separate LPF for each stage where tape degrades the signal. Record and
// playback heads limit bandwidth differently; the feedback path adds further
// darkening on every pass around the loop.
OnePoleLpf record_lpf;
OnePoleLpf playback_lpf;
OnePoleLpf feedback_lpf;
OnePoleLpf noise_lpf;  // smooths white noise from "digital snow" to tape hiss

WhiteNoise tape_noise;

Parameter p_blend, p_feedback, p_record_level, p_tone, p_wow;

// LFO phase accumulators -- never reset to zero. Continuous advance prevents
// the click/kink that a zero-reset would cause. sinf() is periodic, so
// (phase - 2*pi) and phase produce identical output; no loss of precision.
float wow_phase = 0.0f;
float wow_mod_phase =
    0.0f;  // secondary ~0.15 Hz oscillator that drifts wow rate
float flutter_phase = 0.0f;

// Per-parameter smoothers. fonepole convention: smaller coeff = slower.
// The delay smoother gets its coeff from the active profile each block, so
// the pitch-glide feel on KNOB_3 sweeps differs between models.
struct Smoothed {
  float current, target, coeff;
  void Tick() { fonepole(current, target, coeff); }
};

Smoothed s_blend{0.5f, 0.5f, 0.0008f};
Smoothed s_feedback{0.3f, 0.3f, 0.0008f};
Smoothed s_delay{9600.0f, 9600.0f, 0.00008f};  // in samples; starts at 200 ms
Smoothed s_record_level{0.7f, 0.7f, 0.0008f};
Smoothed s_tone{0.5f, 0.5f, 0.0008f};
Smoothed s_wow{0.0f, 0.0f, 0.0008f};

float sample_rate = 48000.0f;

const ModelProfile* active = &kEP2;  // default model at boot

// Delay range min/max in samples, updated each block from TOGGLESWITCH_2.
float delay_min_s = 0.100f * 48000.0f;
float delay_max_s = 0.600f * 48000.0f;

bool bypass = true;
bool sos_mode = false;
bool preamp_only = false;

Led led_mode;    // LED_1: lit when SOS or Preamp-Only mode is active
Led led_bypass;  // LED_2: lit when effect is engaged

// --- Helpers ---

// Equal-power crossfade. cos^2 + sin^2 = 1, so total signal power stays
// constant across the blend sweep. A linear lerp would dip ~3 dB at noon.
static inline void EqualPowerGains(float x, float* dry_gain, float* wet_gain) {
  float angle = x * (kTwoPi * 0.25f);
  *dry_gain = cosf(angle);
  *wet_gain = sinf(angle);
}

// Tube preamp waveshaper: biased tanhf. A DC offset in the tanhf argument
// makes positive and negative half-cycles clip at different rates, generating
// even-order harmonics (2nd, 4th...) -- the "warm" tube sound. Subtracting
// tanhf(asymm) cancels the resulting output DC offset.
// See: https://ccrma.stanford.edu/~jos/pasp/Soft_Clipping.html
static inline float TubeWaveshape(float x, float drive, float asymm) {
  return tanhf(x * drive + asymm) - tanhf(asymm);
}

// Solid-state waveshaper: symmetric tanhf. Odd-order harmonics (3rd, 5th...)
// only. Tighter and more "present" than the asymmetric tube curve.
static inline float SSWaveshape(float x, float drive) {
  return tanhf(x * drive);
}

// Phase accumulator advance. Subtracting one full period (rather than fmodf
// or a zero-reset) keeps the input to sinf() in a well-conditioned range and
// avoids discontinuities at the wrap point.
static inline void AdvancePhase(float* phase, float inc) {
  *phase += inc;
  if (*phase >= kTwoPi) *phase -= kTwoPi;
}

// --- Audio callback ---

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // --- Model select (TOGGLESWITCH_1) ---
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)) {
    case Hothouse::TOGGLESWITCH_UP:
      active = &kEP3;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      active = &kEP2;
      break;
    default:
      active = &kEP1;
      break;
  }

  // --- Delay range (TOGGLESWITCH_2) ---
  // On the real machine, you changed delay range by repositioning the record
  // head relative to the playback head. KNOB_3 (ECHO TIME) sweeps within the
  // chosen range using a log taper, just like the original's tape-position pot.
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)) {
    case Hothouse::TOGGLESWITCH_UP:
      delay_min_s = 0.050f * sample_rate;  // short: slapback and chorus range
      delay_max_s = 0.400f * sample_rate;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      delay_min_s = 0.100f * sample_rate;  // medium: classic rock delay range
      delay_max_s = 0.600f * sample_rate;
      break;
    default:
      delay_min_s = 0.200f * sample_rate;  // long: ambient and dub territory
      delay_max_s = 0.800f * sample_rate;
      break;
  }

  // --- Mode (TOGGLESWITCH_3) ---
  // SOS requires model support; EP-1 silently falls back to normal echo since
  // the erase-head bypass modification didn't exist on most EP-1 units.
  auto mode_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
  sos_mode = (mode_pos == Hothouse::TOGGLESWITCH_UP) && active->sos_capable;
  preamp_only = (mode_pos == Hothouse::TOGGLESWITCH_DOWN);

  // --- Knob targets ---
  s_blend.target = p_blend.Process();
  s_feedback.target = p_feedback.Process();
  s_record_level.target = p_record_level.Process();
  s_tone.target = p_tone.Process();
  s_wow.target = p_wow.Process();

  // Delay time: log taper within the selected range. Read raw (0-1) from
  // KNOB_3 and compute target samples directly -- the Parameter helper can't
  // adapt its range to a toggle switch position.
  float delay_knob = hw.knobs[Hothouse::KNOB_3].Process();
  s_delay.target = delay_min_s * powf(delay_max_s / delay_min_s, delay_knob);

  // Delay smoothing speed is part of the model character. EP-1's heavy tape
  // head moves slowly (long, wandering glide); EP-3's precision motor responds
  // more crisply. This coeff is what makes the pitch glide feel different
  // per model, not just the delay time range.
  s_delay.coeff = active->delay_smooth;

  // Record and feedback LPF cutoffs are fixed for the whole block.
  record_lpf.SetCutoff(active->record_fc, sample_rate);
  feedback_lpf.SetCutoff(active->feedback_fc, sample_rate);

  for (size_t i = 0; i < size; ++i) {
    const float dry_in = in[0][i];

    if (bypass) {
      out[0][i] = out[1][i] = dry_in;
      continue;
    }

    s_blend.Tick();
    s_feedback.Tick();
    s_delay.Tick();
    s_record_level.Tick();
    s_tone.Tick();
    s_wow.Tick();

    // --- 1. Preamp waveshaper ---
    // The Echoplex signal always passes through the preamp, so the coloration
    // is present even with BLEND fully dry. This is historically accurate:
    // the preamp is always in circuit, and many players value it as a
    // standalone tone shaper (especially the EP-3). "Preamp Only" mode
    // exploits exactly this: dry signal through the preamp, no echo added.
    float preamp_out;
    if (active->preamp_asymmetry > 0.05f) {
      // Tube: asymmetric waveshaper generates even-order harmonics.
      preamp_out =
          TubeWaveshape(dry_in, active->preamp_drive, active->preamp_asymmetry);
    } else {
      // Solid state: symmetric odd-harmonic clipping (FET/transistor
      // character).
      preamp_out = SSWaveshape(dry_in, active->preamp_drive);
    }

    // --- 2. Wow and flutter (delay-time modulation) ---
    // Wow is the slow pitch wobble from motor speed variation. A secondary
    // ~0.15 Hz oscillator drifts the wow rate slightly over time, creating
    // "wandering" motor irregularity rather than a steady periodic cycle.
    // Real motor speed is never perfectly constant, and this slow drift (not
    // just the wobble itself) is what separates wow from chorus vibrato.
    AdvancePhase(&wow_mod_phase, kTwoPi * 0.15f / sample_rate);
    float wow_rate_hz = active->wow_rate * (1.0f + 0.25f * sinf(wow_mod_phase));
    AdvancePhase(&wow_phase, kTwoPi * wow_rate_hz / sample_rate);

    // Wow and flutter depths are proportional to the current delay time: the
    // longer the loop, the more absolute pitch variation you get for the same
    // percentage speed error. This matches the physics of the real machine.
    float wow_depth =
        s_delay.current * 0.005f * active->wow_scale * s_wow.current;
    float wow_offset = sinf(wow_phase) * wow_depth;

    // Flutter: higher-frequency (8-10 Hz) tape-transport irregularity, scaled
    // and modeled separately from wow. Both are summed into the read position.
    AdvancePhase(&flutter_phase, kTwoPi * active->flutter_rate / sample_rate);
    float flutter_depth = s_delay.current * kFlutterDepthCoeff *
                          active->flutter_scale * s_wow.current;
    float flutter_offset = sinf(flutter_phase) * flutter_depth;

    float read_pos = fclamp(s_delay.current + wow_offset + flutter_offset, 1.0f,
                            static_cast<float>(kMaxDelaySamples) - 2.0f);

    // --- 3. Read from delay line ---
    // Read BEFORE write so we get the old loop content. If we wrote first,
    // we would read the newly-written sample -- zero-latency feedback, which
    // is wrong and also sounds terrible. Ask me how I know ;)
    delay_line.SetDelay(read_pos);
    float tape_out = delay_line.Read();

    // --- 4. Playback LPF ---
    // TONE knob shifts the cutoff above or below the model's default.
    // Using 10^(knob - 0.5): noon = 1.0x, CCW = ~0.32x (dark), CW = ~3.16x
    // (bright). A log ratio keeps the perceptual step size consistent across
    // the knob range.
    float tone_mult = powf(10.0f, s_tone.current - 0.5f);
    float eff_playback_fc =
        fclamp(active->playback_fc * tone_mult, 400.0f, 18000.0f);
    playback_lpf.SetCutoff(eff_playback_fc, sample_rate);
    float wet = playback_lpf.Process(tape_out);

    // --- 5. Feedback path ---
    // Playback signal routes back to the record input. The dedicated feedback
    // LPF cuts a bit more high end on every pass around the loop -- that's
    // the whole reason Echoplex repeats start relatively bright and gradually
    // turn dark and murky. Without per-pass darkening, the repeats would
    // just sound like a gated digital echo. This one filter IS the sound.
    float fb = feedback_lpf.Process(wet);

    // --- 6. Write to delay line ---
    // New signal (record path) mixed with feedback, then soft-clipped.
    // In SOS mode, feedback is pegged at 1.0 (erase head bypassed: the loop
    // accumulates indefinitely). The SUSTAIN knob is effectively bypassed in
    // SOS; RECORD LEVEL controls how aggressively new signal layers over old.
    float record_signal =
        record_lpf.Process(preamp_out * s_record_level.current);

    float feedback_amt =
        sos_mode ? 1.0f : s_feedback.current * active->max_feedback;

    // tanhf keeps the write value bounded even when feedback_amt >= 1.0.
    // Self-oscillation converges to a stable saturated fixed point rather
    // than an unbounded ramp to NaN. The soft shape also adds a little
    // analog-style warmth to the overloaded loop content.
    delay_line.Write(tanhf(record_signal + fb * feedback_amt));

    // --- 7. Tape hiss ---
    // Low-level filtered white noise. The LPF in main() rolls off the harsh
    // HF to place the hiss in the warm analog zone. Scaling by noise_floor
    // keeps it inaudible at typical listening levels while still adding air.
    float hiss = noise_lpf.Process(tape_noise.Process()) * active->noise_floor;
    wet += hiss;

    // --- 8. Equal-power blend ---
    // In Preamp-Only mode, set wet to zero and blend to 0, so the output is
    // pure preamp_out -- the coloration without any echo.
    float blend = preamp_only ? 0.0f : s_blend.current;
    if (preamp_only) wet = 0.0f;

    float dry_gain, wet_gain;
    EqualPowerGains(blend, &dry_gain, &wet_gain);
    out[0][i] = out[1][i] = preamp_out * dry_gain + wet * wet_gain;
  }
}

// --- Main ---

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // small block keeps modulation latency low
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sample_rate = hw.AudioSampleRate();

  delay_line.Init();
  tape_noise.Init();

  p_blend.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);

  // Allow feedback slightly past 1.0 so players can push into self-oscillation.
  // The max_feedback profile field and the tanhf saturator shape the ceiling
  // per model; the 1.1 ceiling here just ensures the knob gets there.
  p_feedback.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.1f, Parameter::LINEAR);

  // KNOB_3 is read raw in the callback and rescaled by the toggle-selected
  // delay range. No Parameter helper needed.

  // 0.1 to 2.0: CCW is a whisper-quiet record level, CW drives hard saturation.
  p_record_level.Init(hw.knobs[Hothouse::KNOB_4], 0.1f, 2.0f,
                      Parameter::LINEAR);

  p_tone.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  p_wow.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  // Set initial LPF states from the EP-2 default profile.
  record_lpf.SetCutoff(kEP2.record_fc, sample_rate);
  playback_lpf.SetCutoff(kEP2.playback_fc, sample_rate);
  feedback_lpf.SetCutoff(kEP2.feedback_fc, sample_rate);

  // Roll off the harshest HF of the tape noise so it sits in the warm
  // "analog hiss" zone rather than sounding like a spray of digital snow.
  noise_lpf.SetCutoff(4000.0f, sample_rate);

  led_mode.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // LED_1 lights when a non-standard mode is active (SOS or Preamp Only).
    led_mode.Set((sos_mode || preamp_only) ? 1.0f : 0.0f);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_mode.Update();
    led_bypass.Update();

    System::Delay(10);
    hw.CheckResetToBootloader();
  }

  return 0;
}
