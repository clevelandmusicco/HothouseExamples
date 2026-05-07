// TriPhaseTheorem for Hothouse DIY DSP Platform
// Authentic models of three iconic analog phase shifters:
//   - EHX Small Stone (4-stage OTA, exponential LFO response, COLOR feedback)
//   - MXR Phase 90 (4-stage JFET, linear LFO response)
//   - MXR Phase 45 (2-stage JFET, single sweeping notch)
// Copyright (C) 2025 Cleveland Music Co. <code@clevelandmusicco.com>
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

// All three pedals are built from cascaded first-order all-pass filters.
// DaisySP's Phaser class isn't used here; it uses a parallel delay-line
// topology, which is architecturally incompatible with the JFET/OTA all-pass
// cascade these pedals are built around.
//
// Mixing N cascaded all-pass stages 50/50 with the dry signal creates notches
// wherever accumulated phase shift hits an odd multiple of 180 degrees.
// 2 stages: one notch at fc. 4 stages: two notches at ~0.414*fc and ~2.414*fc.
// See: https://en.wikipedia.org/wiki/Phaser_(effect)

#include <math.h>

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisysp::fonepole;
using daisysp::Oscillator;

// --- All-pass stage ---

// One first-order all-pass section. All-pass filters pass all frequencies at
// equal amplitude but shift their phase. Sweeping the break frequency fc with
// an LFO moves that phase-shift curve; mixing the result with the dry signal
// creates the characteristic notches.
struct AllPassStage {
  float a = 0.0f;   // bilinear-transform coefficient, range (-1, 1)
  float z1 = 0.0f;  // one-sample delay state

  // Update the coefficient for a new break frequency fc (Hz).
  void SetFreq(float fc, float fs) {
    float k = tanf(M_PI * fc / fs);
    a = (k - 1.0f) / (k + 1.0f);
  }

  float Process(float in) {
    float out = a * in + z1;
    z1 = in - a * out;  // transposed direct-form II: numerically stable
    return out;
  }
};

// --- Model parameter constants ---

// Phase 45: 2-stage JFET. Single notch sweeps directly at fc.
constexpr float kFcMin45 = 100.0f;
constexpr float kFcMax45 = 2500.0f;

// Phase 90: 4-stage JFET. Same RC network as Phase 45 (24k-ohm + 47nF),
// same fc range, but two simultaneous notches at ~0.414*fc and ~2.414*fc
// produce the more dramatic sound.
constexpr float kFcMin90 = 100.0f;
constexpr float kFcMax90 = 2500.0f;

// Small Stone: 4-stage OTA. The OTA's transconductance (gm ~ Ibias) makes fc
// respond exponentially to the LFO voltage -- that asymmetric "thick swirl"
// character vs. the Phase 90. Range derived from measured notch positions.
constexpr float kFcMinSS = 235.0f;
constexpr float kFcMaxSS = 1800.0f;

// COLOR feedback coefficient. Feeds the last stage's output back to the input,
// sharpening notch resonance (EHX called this "comb filter" mode). 0.35
// approximates the original R27 feedback resistor network value. Crank it up
// to 0.7 for an OTT whooshy thang going on.
constexpr float kColorFeedback = 0.35f;

// Mild soft-saturation applied to the Small Stone all-pass output to model
// the CA3094 OTA's output-stage nonlinearity. At typical guitar signal levels
// this is barely perceptible, but it adds a lil' grind to louder signals.
constexpr float kOtaSatGain = 1.5f;

// Shared LFO rate range (Hz). Log taper: rate = 0.05 * 200^knob.
constexpr float kRateMin = 0.05f;
constexpr float kRateMax = 10.0f;  // kRateMin * powf(200, 1.0)

// --- Phase 45 model ---

struct Phase45Model {
  AllPassStage stages[2];
  Oscillator lfo;
  float sample_rate = 48000.0f;

  void Init(float sr) {
    sample_rate = sr;
    lfo.Init(sr);
    // Triangle LFO: matches the Schmitt-trigger integrator in the original
    // Phase 45/90 circuit, which generates a true triangle wave.
    lfo.SetWaveform(Oscillator::WAVE_TRI);
    lfo.SetAmp(1.0f);
  }

  void SetRate(float hz) { lfo.SetFreq(hz); }

  float Process(float in) {
    float lfo_val = lfo.Process();  // range [-1, 1]

    // Log fc mapping: the sweep spans ~5 octaves (100-2500 Hz), and log scale
    // keeps each octave the same perceptual size across the LFO cycle.
    float normalized = (lfo_val + 1.0f) * 0.5f;
    float fc = kFcMin45 * powf(kFcMax45 / kFcMin45, normalized);

    stages[0].SetFreq(fc, sample_rate);
    stages[1].SetFreq(fc, sample_rate);

    float phased = stages[1].Process(stages[0].Process(in));
    return 0.5f * in + 0.5f * phased;
  }
};

// --- Phase 90 model ---

struct Phase90Model {
  AllPassStage stages[4];
  Oscillator lfo;
  float sample_rate = 48000.0f;
  float rate_hz = 0.5f;

  void Init(float sr) {
    sample_rate = sr;
    lfo.Init(sr);
    lfo.SetWaveform(Oscillator::WAVE_TRI);
    lfo.SetAmp(1.0f);
  }

  void SetRate(float hz) {
    lfo.SetFreq(hz);
    rate_hz = hz;
  }

  float Process(float in) {
    float lfo_val = lfo.Process();

    // Rate-dependent depth: fast sweeps cause phase-modulation artifacts that
    // make the guitar sound out of tune (same physics as vibrato). Scaling the
    // sweep width down as rate climbs keeps perceived intensity roughly
    // constant. Below 2 Hz the full range is used; above that it narrows with
    // sqrt(rate).
    constexpr float kDepthRefRate = 2.0f;
    lfo_val *= fminf(1.0f, sqrtf(kDepthRefRate / rate_hz));

    float normalized = (lfo_val + 1.0f) * 0.5f;
    // Same logarithmic mapping rationale as Phase45 model
    float fc = kFcMin90 * powf(kFcMax90 / kFcMin90, normalized);

    for (int i = 0; i < 4; ++i) stages[i].SetFreq(fc, sample_rate);

    float phased = in;
    for (int i = 0; i < 4; ++i) phased = stages[i].Process(phased);

    return 0.5f * in + 0.5f * phased;
  }
};

// --- Small Stone model ---

struct SmallStoneModel {
  AllPassStage stages[4];
  Oscillator lfo;
  float sample_rate = 48000.0f;
  float fc_center = 651.0f;  // geometric mean of kFcMinSS and kFcMaxSS
  float k_depth = 1.016f;    // log(kFcMaxSS / fc_center)
  float last_output = 0.0f;
  float feedback = 0.0f;  // 0 or kColorFeedback
  float rate_hz = 0.5f;

  void Init(float sr) {
    sample_rate = sr;
    fc_center = sqrtf(kFcMinSS * kFcMaxSS);
    k_depth = logf(kFcMaxSS / fc_center);

    lfo.Init(sr);
    // Sine LFO: the original OTA-based oscillator produces an approximately
    // sinusoidal waveform. The exponential fc mapping (not the LFO shape) is
    // what creates the asymmetric "whomp" feel.
    lfo.SetWaveform(Oscillator::WAVE_SIN);
    lfo.SetAmp(1.0f);
  }

  void SetRate(float hz) {
    lfo.SetFreq(hz);
    rate_hz = hz;
  }

  // COLOR switch engages feedback and sharpens notch resonance.
  void SetColor(bool on) { feedback = on ? kColorFeedback : 0.0f; }

  float Process(float in) {
    float lfo_val = lfo.Process();  // range [-1, 1]

    // Rate-dependent depth: same reasoning as the Phase 90 model -- fast sweeps
    // cause pitch-wobble artifacts. The exponential mapping amplifies this, so
    // the scaling matters at least as much here.
    constexpr float kDepthRefRate = 2.0f;
    lfo_val *= fminf(1.0f, sqrtf(kDepthRefRate / rate_hz));

    // Exponential fc mapping models the OTA's gm ~ Ibias response.
    // At lfo = +1: fc ~= 1800 Hz. At lfo = -1: fc ~= 235 Hz.
    float fc = fc_center * expf(k_depth * lfo_val);

    for (int i = 0; i < 4; ++i) stages[i].SetFreq(fc, sample_rate);

    // Optional feedback path (COLOR switch). Routes last output back to
    // the cascade input, creating resonant peaks between the notches.
    float cascade_in = in + feedback * last_output;

    float phased = cascade_in;
    for (int i = 0; i < 4; ++i) phased = stages[i].Process(phased);

    // Mild OTA saturation models the CA3094 output stage soft-limiting.
    phased = tanhf(phased / kOtaSatGain) * kOtaSatGain;

    last_output = phased;

    return 0.5f * in + 0.5f * phased;
  }
};

// --- Globals ---

Hothouse hw;
Phase45Model phase45;
Phase90Model phase90;
SmallStoneModel small_stone;

Led led_model;   // LED_1: lit when Small Stone is selected (why not, right?)
Led led_bypass;  // LED_2: lit when effect is active

bool bypass = true;
float rate_hz = 0.5f;  // smoothed LFO rate

// --- Audio callback ---

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // KNOB_1: LFO rate. Logarithmic taper for fine control at slow speeds.
  float knob = hw.knobs[Hothouse::KNOB_1].Process();
  float rate_target = kRateMin * powf(kRateMax / kRateMin, knob);
  fonepole(rate_hz, rate_target, 0.001f);

  // Update LFO rate on all three models. They all run continuously so that
  // switching models mid-sweep produces no pops or phase discontinuities.
  phase45.SetRate(rate_hz);
  phase90.SetRate(rate_hz);
  small_stone.SetRate(rate_hz);

  // TOGGLESWITCH_1 selects the active pedal model.
  Hothouse::ToggleswitchPosition model_pos =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);

  // TOGGLESWITCH_2 controls the Small Stone COLOR switch.
  // DOWN = COLOR off; MIDDLE or UP = COLOR on.
  Hothouse::ToggleswitchPosition color_pos =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
  small_stone.SetColor(color_pos != Hothouse::TOGGLESWITCH_DOWN);

  for (size_t i = 0; i < size; ++i) {
    float dry = in[0][i];
    float wet = dry;

    if (!bypass) {
      switch (model_pos) {
        case Hothouse::TOGGLESWITCH_UP:
          wet = small_stone.Process(dry);
          break;
        case Hothouse::TOGGLESWITCH_MIDDLE:
          wet = phase90.Process(dry);
          break;
        case Hothouse::TOGGLESWITCH_DOWN:
        default:
          wet = phase45.Process(dry);
          break;
      }
    }

    out[0][i] = out[1][i] = wet;
  }
}

// --- Main ---

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float sample_rate = hw.AudioSampleRate();

  phase45.Init(sample_rate);
  phase90.Init(sample_rate);
  small_stone.Init(sample_rate);

  led_model.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    bool small_stone_active =
        hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1) ==
        Hothouse::TOGGLESWITCH_UP;

    led_model.Set(small_stone_active ? 1.0f : 0.0f);
    led_bypass.Set(bypass ? 0.0f : 1.0f);

    led_model.Update();
    led_bypass.Update();

    hw.CheckResetToBootloader();
  }

  return 0;
}
