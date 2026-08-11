// EchoKingMkII for Cleveland Music Co. Hothouse DIY DSP Platform
// DSP models of the Maestro Echoplex family: EP-1, EP-2, and EP-3
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
//
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

// 800 ms covers the long delay plus wow/flutter excursion; ~150 KB in SDRAM.
constexpr float kMaxDelaySeconds = 0.8f;
constexpr size_t kMaxDelaySamples =
    static_cast<size_t>(48000.0f * kMaxDelaySeconds);

constexpr float kTwoPi = 6.28318530717958647692f;

// Flutter depth scalar relative to current delay length. Audible range is
// roughly 0.0f-0.002f; start small.
constexpr float kFlutterDepthCoeff = 0.0004f;

// SOS loop gain, just under unity. Tape stage is normalised to 1.0, so this
// is the actual loop gain.
constexpr float kSosFeedback = 0.985f;

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

struct ModelProfile {
  float preamp_drive;      // input gain into the waveshaper
  bool is_tube;            // true: cascaded triode; false: EP-3 preamp
  float preamp_asymmetry;  // even-harmonic bias; ignored when !is_tube
  float preamp_makeup;     // post-preamp level trim to keep model swaps
                           // within ~2 dB of each other
  float record_fc;         // record-path LPF cutoff, Hz
  float playback_fc;       // playback-path LPF cutoff, Hz
  float feedback_fc;       // feedback-path LPF cutoff, Hz (per-pass darkening)
  float transport_irregularity;  // multiplier on BOTH wow and flutter depths
  float wow_rate;                // wow LFO base rate, Hz
  float flutter_rate;            // flutter LFO rate, Hz
  float noise_floor;       // tape hiss amplitude (linear scale on white noise)
  float feedback_ceiling;  // actual loop gain when SUSTAIN knob is at max
  float delay_smooth;      // one-pole coeff for delay-time glide
  float tape_drive;    // scaler into the variable-knee curve; normalised out of
                       // small-signal gain so it shapes the knee only
  int tape_knee;       // n in x / (1+|x|^n)^(1/n); legal values: 2, 3, 4
  float tape_erasure;  // 0-1; how far the bias-erasure LPF drops at peak
  float tape_hyst;     // sign-of-derivative bias depth for the hysteresis mode
  bool sos_capable;    // SOS mode available for this model
};

// EP-1: tube (12AX7 single-ended), early cartridge transport. Loosest
// mechanics, strongest even-harmonic coloration, most wow and flutter.
// The "vintage instability" model. No SOS -- that came later.
constexpr ModelProfile kEP1 = {
    1.4f,     true,
    0.22f,     // single-ended triode, strong 2nd harmonic
    1.25f,     // lift the quietest preamp to near reference
    5500.0f,   // slow tape formulation, less precise bias = soft HF
    4500.0f,   // early head gap loss adds more rolloff at playback
    3500.0f,   // repeats go quite dark on each pass
    1.40f,     // imprecise AC motor = deep pitch wander
    1.2f,      // wow_rate, Hz
    8.0f,      // flutter_rate, Hz
    0.0002f,   // early Ampex oxide, notably hissy
    0.95f,     // approaches but doesn't quite reach unity
    0.00006f,  // slow, heavy tape head -- long pitch glide
    1.40f,     // pushes the curve hardest of the three
    2,         // softest knee -- oldest, loosest tape
    0.55f,     // heavy bias drop under loud signal
    0.040f,    // strongest M-H stickiness, drifty old oxide
    false,
};

// EP-2: tube (12AX7 + 12AU7), revised circuit. The canonical Echoplex sound.
// Thick, warm, the dry tone is perceptibly colored even without any echo.
// Sound-on-sound was available on later EP-2 units.
constexpr ModelProfile kEP2 = {
    1.5f,  // slightly more drive for the "thick" EP-2 feel
    true,     0.18f,
    1.00f,  // EP-2 is the level reference
    6000.0f,  5000.0f, 4000.0f,
    1.00f,  // reference (mid-era AC motor)
    1.3f,   // wow_rate, Hz
    9.0f,   // flutter_rate, Hz
    0.0001f,
    1.00f,  // parks right at unity -- musical sustain
    0.00008f,
    1.25f,   // canonical "warm but not breaking" level
    3,       // midway between EP-1 and EP-3
    0.40f,   // moderate bias erasure
    0.025f,  // mild stickiness
    true,
};

// EP-3: solid state (JFET/FET), mid-1970s. Tighter, brighter, more focused.
// The preamp doubles as a standalone tone shaper. Better motor regulation
// means much less wow/flutter. SOS supported.
constexpr ModelProfile kEP3 = {
    1.2f,     // input level into the EP-3 preamp shaper
    false,    // uses the EP-3 2N5172 preamp model
    0.0f,     // unused when is_tube == false
    0.75f,    // trim the famous boost down so it sits ~+3 dB
              // over EP-2 instead of ~+5 dB
    7500.0f,  // solid-state record amp captures more high end
    6000.0f,
    5000.0f,   // repeats stay comparatively bright
    0.65f,     // regulated motor = much less wander
    1.5f,      // wow_rate, Hz
    10.5f,     // flutter_rate, Hz
    0.0f,      // lower floor (no tube heater current)
    1.05f,     // slightly past unity for lively runaway
    0.00012f,  // precision motor, crisper pitch on time change
    1.10f,     // tightest, least pushed into the curve
    4,         // firmest shoulder
    0.20f,     // better bias regulation, less HF loss under drive
    0.012f,    // cleanest oxide, minimal M-H memory
    true,
};

// Tape-age deformation. Multipliers on the active model's tape_* and transport
// fields; knee_delta shifts tape_knee by +/-1 (clamped to [2,4]). Stock is
// identity. Composes orthogonally with model select.
struct TapeAge {
  float drive_mult;
  float erasure_mult;
  float hyst_mult;
  float transport_mult;  // scales transport_irregularity (wow + flutter depth)
  float flutter_rate_mult;  // worn drive train flutters faster
  int knee_delta;
};

// New tape: less drive into the curve, firmer knee, cleaner erasure, low
// hysteresis, calm transport. Knee +1 clamps for EP-3 (already at 4).
constexpr TapeAge kAgeNew = {0.85f, 0.70f, 0.55f, 0.75f, 0.95f, +1};

constexpr TapeAge kAgeStock = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0};  // identity

// Heavily-worn: drive up (lost headroom), erasure way up (HF dropout),
// hysteresis up (sticky oxide), transport irregular and slightly faster
// flutter. Knee -1 clamps for EP-1 (already at 2).
constexpr TapeAge kAgeWorn = {1.25f, 1.50f, 1.75f, 1.35f, 1.10f, -1};

// --- Delay buffer ---

// In external SDRAM; doesn't fit on-chip SRAM.
DelayLine<float, kMaxDelaySamples> DSY_SDRAM_BSS delay_line;

// --- One-pole LPF ---

// y[n] = (1-a)*x[n] + a*y[n-1]; a near 1 = heavy rolloff.
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

// Per-stage LPFs. See ModelProfile fields for cutoff roles.
OnePoleLpf record_lpf;
OnePoleLpf playback_lpf;
OnePoleLpf feedback_lpf;
OnePoleLpf noise_lpf;  // smooths white noise from "digital snow" to tape hiss

WhiteNoise tape_noise;

Parameter p_blend, p_feedback, p_record_level, p_tone, p_wow;

// Continuous phase accumulators; never reset (avoids clicks at the wrap).
float wow_phase = 0.0f;
float wow_mod_phase = 0.0f;  // ~0.15 Hz secondary, drifts wow rate
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

float sample_rate = 0.0f;  // set in main() from hw.AudioSampleRate()

const ModelProfile* active_model = &kEP2;  // default model at boot
const TapeAge* active_age = &kAgeStock;    // default age at boot

// Delay range in samples, 50 ms to 800 ms. Set in main() once sample_rate is
// known.
float delay_min_samples = 0.0f;
float delay_max_samples = 0.0f;

bool bypass = true;
bool sos_mode = false;
bool preamp_only = false;

Led led_mode;    // LED_1: lit when SOS or Preamp-Only mode is active
Led led_bypass;  // LED_2: lit when effect is engaged

// --- Helpers ---

// Equal-power crossfade: cos^2 + sin^2 = 1 keeps total power constant.
static inline void EqualPowerGains(float x, float* dry_gain, float* wet_gain) {
  float angle = x * (kTwoPi * 0.25f);
  *dry_gain = cosf(angle);
  *wet_gain = sinf(angle);
}

// Two-stage triode model with an interstage DC-block (~38 Hz HPF, R=0.995).
// Stage 1 carries the full asymmetric bias; stage 2 re-saturates the
// DC-cleaned signal at smaller bias so the offset doesn't stack.
struct CascadedTube {
  float dc_x_prev = 0.0f;
  float dc_y_prev = 0.0f;
  static constexpr float kDcBlockR = 0.995f;

  float Process(float x, float drive, float asymm) {
    // Stage 1: biased tanh at moderate drive.
    const float s1 = tanhf(x * drive * 0.7f + asymm) - tanhf(asymm);

    // DC-block: y[n] = x[n] - x[n-1] + R * y[n-1].
    const float dc_clean = s1 - dc_x_prev + kDcBlockR * dc_y_prev;
    dc_x_prev = s1;
    dc_y_prev = dc_clean;

    // Stage 2: re-saturates the cleaned-up signal at lower bias.
    const float small_asymm = asymm * 0.3f;
    return tanhf(dc_clean * drive * 0.9f + small_asymm) - tanhf(small_asymm);
  }
};

CascadedTube cascaded_tube;

// EP-3 preamp model: single-stage common-emitter NPN (2N5172) with feedback
// biasing. Emitter-bypass low-shelf cut (~120 Hz, ~6 dB), asymmetric Class-A
// waveshaper, Miller-cap HF rolloff (~9 kHz), and ~+4 dB makeup.
struct EP3Preamp {
  OnePoleLpf shelf_lpf;
  OnePoleLpf miller_lpf;
  float dc_x_prev = 0.0f;
  float dc_y_prev = 0.0f;

  static constexpr float kDcBlockR = 0.997f;
  static constexpr float kShelfDepth = 0.50f;  // ~6 dB low-shelf cut
  static constexpr float kBias = 0.12f;        // class-A operating offset
  static constexpr float kMakeup = 1.6f;       // ~+4 dB famous boost
  static constexpr float kShelfFc = 120.0f;    // emitter-bypass shelf corner
  static constexpr float kMillerFc = 9000.0f;  // C_cb * R_c rolloff

  void Init(float fs) {
    shelf_lpf.SetCutoff(kShelfFc, fs);
    miller_lpf.SetCutoff(kMillerFc, fs);
  }

  float Process(float x, float drive) {
    // 1. Emitter-bypass low-shelf: subtracting a fraction of the LF content
    // attenuates lows relative to mids/highs by ~6 dB below the corner.
    const float lf = shelf_lpf.Process(x);
    const float shaped_in = x - kShelfDepth * lf;

    // 2. Asymmetric Class-A waveshaper. The static bias offsets the
    // operating point off-center on the tanh curve, so positive and
    // negative swings clip at different rates -- the 2nd-harmonic
    // mechanism. Subtracting tanhf(kBias) cancels the static DC offset;
    // a level-dependent residual remains and is cleaned up next.
    const float driven = shaped_in * drive + kBias;
    const float shaped = tanhf(driven) - tanhf(kBias);

    // 3. DC blocker for drive-dependent residual offset.
    const float dc_clean = shaped - dc_x_prev + kDcBlockR * dc_y_prev;
    dc_x_prev = shaped;
    dc_y_prev = dc_clean;

    // 4. Miller-cap HF rolloff. 5. Makeup gain for the in-path boost.
    return miller_lpf.Process(dc_clean) * kMakeup;
  }
};

EP3Preamp ep3_preamp;

// Tape write-path saturation. Three layers always run: variable-knee static
// curve, level-dependent HF rolloff (bias erasure), and envelope-gated
// hysteresis bias. Curve: y = tanhf(x_d / (1 + |x_d|^n)^(1/n)) / tape_drive,
// x_d = x * tape_drive. tape_knee picks n from {2,3,4} so the hot path uses
// sqrtf / cbrtf / nested sqrtf instead of powf.
struct TapeStage {
  OnePoleLpf bias_lpf;
  float env = 0.0f;         // rectified-signal envelope follower state
  float x_prev = 0.0f;      // previous input sample for sign(dx)
  float hyst_state = 0.0f;  // smoothed sign(dx), the actual hysteresis bias
  int cutoff_counter = 0;   // counts samples between SetCutoff calls

  static constexpr float kAttackCoeff = 0.016f;    // ~3 ms at 48 kHz
  static constexpr float kReleaseCoeff = 0.0016f;  // ~30 ms at 48 kHz
  static constexpr float kHystSmooth = 0.02f;      // ~1 ms LPF on sign(dx)
  static constexpr float kMinCutoff = 800.0f;
  static constexpr int kCutoffUpdateInterval = 4;

  float Process(float x, const ModelProfile* prof, const TapeAge* tape_age,
                float fs) {
    const float abs_x = fabsf(x);
    const float env_coeff = (abs_x > env) ? kAttackCoeff : kReleaseCoeff;
    env += env_coeff * (abs_x - env);

    const float dx = x - x_prev;
    x_prev = x;

    const float sign_dx = (dx > 0.0f) ? 1.0f : (dx < 0.0f ? -1.0f : 0.0f);
    hyst_state += kHystSmooth * (sign_dx - hyst_state);

    // Effective drive folds tape_age->drive_mult in, and the normalisation at
    // the return uses the same product so small-signal gain stays at 1.0.
    const float eff_drive = prof->tape_drive * tape_age->drive_mult;
    const float eff_hyst = prof->tape_hyst * tape_age->hyst_mult;
    const float eff_erasure = prof->tape_erasure * tape_age->erasure_mult;

    // env multiplier zeroes the bias during silence even if hyst_state has
    // not decayed, so the hysteresis term doesn't inject audible noise into
    // the tails of decaying repeats.
    float x_d = x * eff_drive + eff_hyst * env * hyst_state;

    if (++cutoff_counter >= kCutoffUpdateInterval) {
      cutoff_counter = 0;
      float fc = prof->record_fc * (1.0f - eff_erasure * env);
      if (fc < kMinCutoff) fc = kMinCutoff;
      bias_lpf.SetCutoff(fc, fs);
    }
    x_d = bias_lpf.Process(x_d);

    // Integer exponent lets the hot loop use sqrtf/cbrtf instead of powf.
    int n = prof->tape_knee + tape_age->knee_delta;
    if (n < 2)
      n = 2;
    else if (n > 4)
      n = 4;
    const float abs_xd = fabsf(x_d);
    float denom;
    if (n == 2) {
      denom = sqrtf(1.0f + abs_xd * abs_xd);
    } else if (n == 3) {
      const float a3 = abs_xd * abs_xd * abs_xd;
      denom = cbrtf(1.0f + a3);
    } else {
      const float x2 = abs_xd * abs_xd;
      const float x4 = x2 * x2;
      denom = sqrtf(sqrtf(1.0f + x4));
    }
    // /eff_drive normalises small-signal gain to 1.0 so drive shapes the knee
    // without secretly amplifying the feedback loop. tanhf bounds the
    // asymptote so high-feedback loops settle short of the rail.
    return tanhf(x_d / denom) / eff_drive;
  }
};

TapeStage tape_stage;

// Wrap by subtraction so sinf() input stays well-conditioned.
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
      active_model = &kEP3;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      active_model = &kEP2;
      break;
    default:
      active_model = &kEP1;
      break;
  }

  // --- Tape age (TOGGLESWITCH_2) ---
  // UP = new tape, MIDDLE = stock (model defaults), DOWN = heavily-worn.
  // Composes with the model select above without special-casing.
  switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)) {
    case Hothouse::TOGGLESWITCH_UP:
      active_age = &kAgeNew;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      active_age = &kAgeStock;
      break;
    default:
      active_age = &kAgeWorn;
      break;
  }

  // --- Mode (TOGGLESWITCH_3) ---
  // SOS requires model support; EP-1 silently falls back to normal echo since
  // the erase-head bypass didn't exist on EP-1 units.
  auto mode_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
  sos_mode =
      (mode_pos == Hothouse::TOGGLESWITCH_UP) && active_model->sos_capable;
  preamp_only = (mode_pos == Hothouse::TOGGLESWITCH_DOWN);

  // --- Knob targets ---
  s_blend.target = p_blend.Process();
  s_feedback.target = p_feedback.Process();
  s_record_level.target = p_record_level.Process();
  s_tone.target = p_tone.Process();
  s_wow.target = p_wow.Process();

  // Delay time: log taper across the fixed 50-800 ms range. Read raw (0-1)
  // from KNOB_3 and compute target samples directly -- the Parameter helper
  // can't express a log mapping.
  float delay_knob = hw.knobs[Hothouse::KNOB_3].Process();
  s_delay.target = delay_min_samples *
                   powf(delay_max_samples / delay_min_samples, delay_knob);

  // Delay-smoothing coeff is model-dependent: EP-1's heavy head glides
  // slowly, EP-3's precision motor responds crisply. The per-model glide
  // feel comes from this, not the time range.
  s_delay.coeff = active_model->delay_smooth;

  // Record and feedback LPF cutoffs are fixed for the whole block.
  record_lpf.SetCutoff(active_model->record_fc, sample_rate);
  feedback_lpf.SetCutoff(active_model->feedback_fc, sample_rate);

  // Trails: the tape loop keeps turning when bypassed or in Preamp-Only, but
  // nothing new is recorded onto it.
  const bool feeding = !bypass && !preamp_only;

  for (size_t i = 0; i < size; ++i) {
    const float dry_in = in[0][i];

    s_blend.Tick();
    s_feedback.Tick();
    s_delay.Tick();
    s_record_level.Tick();
    s_tone.Tick();
    s_wow.Tick();

    // --- 1. Preamp waveshaper ---
    // Always in path, even with BLEND fully dry: matches the real machine
    // and lets Preamp-Only mode use the preamp as a standalone tone shaper.
    // Keeps running (result discarded) when bypassed so its DC blockers stay
    // settled and re-engaging doesn't thump.
    float preamp_out;
    if (active_model->is_tube) {
      preamp_out = cascaded_tube.Process(dry_in, active_model->preamp_drive,
                                         active_model->preamp_asymmetry);
    } else {
      preamp_out = ep3_preamp.Process(dry_in, active_model->preamp_drive);
    }
    // Per-model post-trim. Sits outside the waveshapers so the saturation
    // character stays intact; only applied when not in Preamp-Only mode.
    if (!preamp_only) preamp_out *= active_model->preamp_makeup;

    // --- 2. Wow and flutter (delay-time modulation) ---
    // Nested LFO drifts wow rate; depths scale with delay time. See README.
    AdvancePhase(&wow_mod_phase, kTwoPi * 0.15f / sample_rate);
    float wow_rate_hz =
        active_model->wow_rate * (1.0f + 0.25f * sinf(wow_mod_phase));
    AdvancePhase(&wow_phase, kTwoPi * wow_rate_hz / sample_rate);

    const float eff_transport =
        active_model->transport_irregularity * active_age->transport_mult;
    float wow_depth = s_delay.current * 0.005f * eff_transport * s_wow.current;
    float wow_offset = sinf(wow_phase) * wow_depth;

    AdvancePhase(&flutter_phase, kTwoPi * active_model->flutter_rate *
                                     active_age->flutter_rate_mult /
                                     sample_rate);
    float flutter_depth =
        s_delay.current * kFlutterDepthCoeff * eff_transport * s_wow.current;
    float flutter_offset = sinf(flutter_phase) * flutter_depth;

    float read_pos = fclamp(s_delay.current + wow_offset + flutter_offset, 1.0f,
                            static_cast<float>(kMaxDelaySamples) - 2.0f);

    // --- 3-7. Wet path ---
    // Read before write so the read sees old loop content.
    delay_line.SetDelay(read_pos);
    float tape_out = delay_line.Read();

    // TONE multiplies the model cutoff by 10^(knob - 0.5).
    float tone_mult = powf(10.0f, s_tone.current - 0.5f);
    float eff_playback_fc =
        fclamp(active_model->playback_fc * tone_mult, 400.0f, 18000.0f);
    playback_lpf.SetCutoff(eff_playback_fc, sample_rate);
    float wet = playback_lpf.Process(tape_out);

    // Per-pass HF rolloff darkens repeats around the loop.
    float fb = feedback_lpf.Process(wet);

    // Zero into the record LPF (rather than skipping it) lets the record path
    // ring down instead of stepping when the input is cut.
    float record_signal = record_lpf.Process(
        feeding ? preamp_out * s_record_level.current : 0.0f);

    // SOS parks feedback just under unity (erase head bypassed).
    float feedback_amt =
        sos_mode ? kSosFeedback
                 : s_feedback.current * active_model->feedback_ceiling;
    const float write_in = record_signal + fb * feedback_amt;
    float write_out =
        tape_stage.Process(write_in, active_model, active_age, sample_rate);

    // Flush to true zero once a decaying trail goes inaudible: avoids denormal
    // stalls and guarantees the buffer actually empties within one delay pass.
    if (fabsf(write_out) < 1e-12f) write_out = 0.0f;
    delay_line.Write(write_out);

    // Hiss is a property of the engaged machine; a bypassed pedal shouldn't
    // add a noise floor once its trail has died away.
    if (!bypass) {
      float hiss =
          noise_lpf.Process(tape_noise.Process()) * active_model->noise_floor;
      wet += hiss;
    }

    // --- 8. Equal-power blend ---
    float blend = preamp_only ? 0.0f : s_blend.current;
    float dry_gain, wet_gain;
    EqualPowerGains(blend, &dry_gain, &wet_gain);

    if (bypass) {
      // Trails: dry is clean at unity (no preamp), only the decaying loop
      // rides on top at the BLEND wet gain.
      out[0][i] = out[1][i] = dry_in + wet * wet_gain;
    } else {
      out[0][i] = out[1][i] = preamp_out * dry_gain + wet * wet_gain;
    }
  }
}

// --- Main ---

int main() {
  hw.Init();
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.SetAudioBlockSize(4);  // small block keeps modulation latency low
  sample_rate = hw.AudioSampleRate();

  // Recompute the fixed delay range from the actual hardware sample rate,
  // in case it ever differs from the 48 kHz literal used at file scope.
  delay_min_samples = 0.05f * sample_rate;
  delay_max_samples = 0.80f * sample_rate;

  delay_line.Init();
  tape_noise.Init();

  p_blend.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);

  // Knob position 0..1 multiplies the per-model feedback_ceiling to give the
  // actual loop gain. Whether SUSTAIN at max parks below, at, or past unity
  // is set by the profile, not by the knob range.
  p_feedback.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);

  // 0.1 to 2.0: CCW is a whisper-quiet record level, CW drives hard saturation.
  p_record_level.Init(hw.knobs[Hothouse::KNOB_4], 0.1f, 2.0f,
                      Parameter::LINEAR);

  p_tone.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  p_wow.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  // record/playback/feedback LPF cutoffs are set in AudioCallback before use.
  // Only the bias-erasure pole needs priming here (in-loop SetCutoff runs
  // every 4 samples, so the first few samples would otherwise see a=0).
  tape_stage.bias_lpf.SetCutoff(kEP2.record_fc, sample_rate);

  // EP-3 preamp shelf and Miller-cap poles. Fixed cutoffs; no in-loop
  // SetCutoff required.
  ep3_preamp.Init(sample_rate);

  // Roll off the harshest HF of the tape noise so it sits in the warm
  // "analog hiss" zone rather than sounding like a spray of digital snow.
  noise_lpf.SetCutoff(4000.0f, sample_rate);

  led_mode.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // LED_1: blinks 2 Hz on/off in SOS, solid on for Preamp-Only, off
    // otherwise.
    bool led_mode_on;
    if (sos_mode) {
      led_mode_on = (System::GetNow() % 500) < 250;
    } else {
      led_mode_on = preamp_only;
    }
    led_mode.Set(led_mode_on ? 1.0f : 0.0f);
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_mode.Update();
    led_bypass.Update();

    System::Delay(10);
    hw.CheckResetToBootloader();
  }
}
