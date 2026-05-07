// Glowjob Photon Vibe for Hothouse DIY DSP Platform
// Reasonably accurate model of the classic Shin-ei Uni-Vibe (1968) photocell
// phaser/chorus with popular mods (... a three-pint educational experiment)
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

// --- Effect brief ---

// The Uni-Vibe is NOT a standard phaser. It's a 4-stage all-pass cascade where
// each stage has a *different* capacitor value (0.015, 0.022, 0.047, 0.100 uF
// from the Shin-ei schematic). A lamp and four LDRs (photoresistors) modulate
// all stages simultaneously, but the unequal cap values put the notches at
// very different frequencies -- that dense, uneven clustering is the sound.
//
// The other thing that makes it "wobbly" rather than "swirly": LDRs are slow
// to track light changes, and they're asymmetric -- faster to drop resistance
// (brightening lamp) than to raise it (dimming lamp). That lopsided lag
// distorts the sine LFO into a skewed waveform unique to the circuit.
// See: https://www.electrosmash.com/uni-vibe

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

// First-order all-pass section, same topology as the RC network in the
// original circuit. Shifts phase without changing amplitude.
// See: https://ccrma.stanford.edu/~jos/filters/First_Order_Allpass_Filters.html
struct AllPassStage {
  float a = 0.0f;
  float z1 = 0.0f;

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

// --- LDR photocell model ---

// Models a lamp-driven LDR (light-dependent resistor) with asymmetric lag.
// Real LDRs respond faster to increasing light (lamp brightening, resistance
// dropping) than to decreasing light (lamp dimming, resistance rising). That
// asymmetry skews the sine LFO into the characteristic UniVibe "wobble".
struct LdrModel {
  float state = 0.5f;          // current LDR "openness": 0 = dark, 1 = bright
  float attack_coeff = 0.01f;  // coeff for rising (lamp brightening) -- faster
  float release_coeff = 0.002f;  // coeff for falling (lamp dimming) -- slower

  // lag_knob: 0-1, maps lag cutoff from 200 Hz (barely any lag) down to 1 Hz
  // (sluggish). ratio: how much faster attack is vs. release (>1 = more asymm).
  void SetLag(float lag_knob, float ratio, float fs) {
    // Exponential mapping: lots of audible lag in the middle of the knob range
    float lag_freq = 200.0f * powf(1.0f / 200.0f, lag_knob);
    float base_coeff = 2.0f * M_PI * lag_freq / fs;
    release_coeff = base_coeff;
    attack_coeff = base_coeff * ratio;
  }

  float Process(float lamp) {
    float coeff = (lamp > state) ? attack_coeff : release_coeff;
    fonepole(state, lamp, coeff);
    return state;
  }
};

// Capacitor ratios from the Shin-ei schematic: 0.015, 0.022, 0.047, 0.100 uF.
// Stage 1 sets the reference frequency; the others are proportionally lower.
constexpr float kCapRatios[4] = {1.0f, 0.6818f, 0.3191f, 0.15f};

// Stage 1 sweep range in Hz. Derived from the original RC network:
// R_total = 22k (fixed) + LDR (500 ohm to 150k), C1 = 0.015 uF.
constexpr float kFcSweepMin = 60.0f;
constexpr float kFcSweepMax = 480.0f;

// --- UniVibe model ---

struct UniVibeModel {
  AllPassStage stages[4];
  Oscillator lfo;
  LdrModel ldr;
  float sample_rate = 48000.0f;
  float depth = 1.0f;
  float feedback_amt = 0.0f;
  float feedback_state = 0.0f;
  float mix = 0.5f;
  float volume = 1.0f;
  bool vibrato_mode = false;

  void Init(float sr) {
    sample_rate = sr;
    lfo.Init(sr);
    // Sine LFO models the approximately sinusoidal lamp oscillator in the
    // original circuit. The LDR lag (not the LFO shape) creates the skew.
    lfo.SetWaveform(Oscillator::WAVE_SIN);
    lfo.SetAmp(1.0f);
  }

  void SetSpeed(float hz) { lfo.SetFreq(hz); }
  void SetDepth(float d) { depth = d; }
  void SetFeedback(float f) { feedback_amt = f; }
  void SetMix(float m) { mix = m; }
  void SetVolume(float v) { volume = v; }
  void SetVibratoMode(bool v) { vibrato_mode = v; }

  void SetLag(float lag_knob, float ratio) {
    ldr.SetLag(lag_knob, ratio, sample_rate);
  }

  float Process(float in) {
    float lfo_val = lfo.Process();  // [-1, 1], models lamp brightness

    // Scale LFO by depth and center it in [0, 1] lamp range
    float lamp = 0.5f + lfo_val * 0.5f * depth;

    // LDR tracks lamp with asymmetric lag -- this is the secret sauce
    float ldr_state = ldr.Process(lamp);

    // Map LDR state to stage-1 sweep frequency (log scale, same reasoning as
    // other phaser models: each octave occupies equal perceptual space)
    float fc1 = kFcSweepMin * powf(kFcSweepMax / kFcSweepMin, ldr_state);
    for (int i = 0; i < 4; ++i) {
      stages[i].SetFreq(fc1 * kCapRatios[i], sample_rate);
    }

    // Optional feedback: routes stage-4 output back to stage-1 input.
    // Not in the original circuit -- sharpens notch resonance into a more
    // gnarly, nasal sound.
    float cascade_in = in + feedback_amt * feedback_state;

    float wet = cascade_in;
    for (int i = 0; i < 4; ++i) wet = stages[i].Process(wet);
    feedback_state = wet;

    // Chorus: 50/50 (or user-controlled) wet+dry mix creates comb-filter
    // notches. Vibrato: wet-only signal, pure pitch modulation with no dry.
    float output;
    if (vibrato_mode) {
      output = wet;
    } else {
      output = mix * wet + (1.0f - mix) * in;
    }

    return output * volume;
  }
};

// --- Globals ---

Hothouse hw;
UniVibeModel univibe;

Led led_mode;    // LED_1: lit when Vibrato mode is active
Led led_bypass;  // LED_2: lit when effect is active (not bypassed)

bool bypass = true;

// Smoothed control values (avoid zipper noise on knobs)
float speed_hz = 1.0f;
float depth_val = 0.5f;
float lag_val = 0.5f;
float mix_val = 0.5f;
float volume_val = 1.0f;
float feedback_val = 0.0f;

// LFO rate range; log taper so slow speeds have fine control
constexpr float kSpeedMin = 0.5f;
constexpr float kSpeedMax = 10.0f;

// Max feedback depth. Values above ~0.7 can get unstable -- be careful.
constexpr float kFeedbackMax = 0.65f;

// --- Audio callback ---

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // KNOB_1: Speed. Log taper: slow speeds (0.5-2 Hz) occupy most of the range.
  float speed_target = kSpeedMin * powf(kSpeedMax / kSpeedMin,
                                        hw.knobs[Hothouse::KNOB_1].Process());
  fonepole(speed_hz, speed_target, 0.001f);
  univibe.SetSpeed(speed_hz);

  // KNOB_2: Intensity/Depth.
  fonepole(depth_val, hw.knobs[Hothouse::KNOB_2].Process(), 0.001f);
  univibe.SetDepth(depth_val);

  // KNOB_5: Volume (output gain, up to +3dB headroom for Vibrato mode which
  // can sound quieter than Chorus due to the missing dry signal).
  fonepole(volume_val, hw.knobs[Hothouse::KNOB_5].Process() * 1.5f, 0.001f);
  univibe.SetVolume(volume_val);

  // KNOB_4: Wet/dry mix (Chorus mode only). CCW = dry, CW = fully wet.
  // A 50/50 blend (knob at noon) is the classic Chorus setting.
  fonepole(mix_val, hw.knobs[Hothouse::KNOB_4].Process(), 0.001f);
  univibe.SetMix(mix_val);

  // KNOB_6: Feedback depth (only active when TOGGLESWITCH_2 engages it)
  bool feedback_on = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2) !=
                     Hothouse::TOGGLESWITCH_DOWN;
  float feedback_target =
      feedback_on ? hw.knobs[Hothouse::KNOB_6].Process() * kFeedbackMax : 0.0f;
  fonepole(feedback_val, feedback_target, 0.001f);
  univibe.SetFeedback(feedback_val);

  // TOGGLESWITCH_1: Chorus (UP or MIDDLE) vs Vibrato (DOWN)
  bool vibrato = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1) ==
                 Hothouse::TOGGLESWITCH_DOWN;
  univibe.SetVibratoMode(vibrato);

  // KNOB_3 + TOGGLESWITCH_3: Photocell lag amount and attack/release asymmetry.
  // TOGGLESWITCH_3 selects the model of the LDR response:
  //   UP    = Modern (3:1 ratio): subtle asymmetry, cleaner sweep
  //   MIDDLE = Vintage Shin-ei (6:1 ratio): classic lopsided wobble
  //   DOWN  = Sluggish (12:1 ratio): exaggerated, almost tremolo-like
  Hothouse::ToggleswitchPosition ldr_char =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
  float ldr_ratio;
  switch (ldr_char) {
    case Hothouse::TOGGLESWITCH_UP:
      ldr_ratio = 3.0f;
      break;
    case Hothouse::TOGGLESWITCH_MIDDLE:
      ldr_ratio = 6.0f;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
    default:
      ldr_ratio = 12.0f;
      break;
  }
  float lag_target = hw.knobs[Hothouse::KNOB_3].Process();
  fonepole(lag_val, lag_target, 0.001f);
  univibe.SetLag(lag_val, ldr_ratio);

  for (size_t i = 0; i < size; ++i) {
    float dry = in[0][i];
    float wet = bypass ? dry : univibe.Process(dry);
    out[0][i] = out[1][i] = wet;
  }
}

// --- Main ---

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float sr = hw.AudioSampleRate();

  univibe.Init(sr);

  led_mode.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    bool vibrato_active =
        hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1) ==
        Hothouse::TOGGLESWITCH_DOWN;

    led_mode.Set(vibrato_active ? 1.0f : 0.0f);
    led_bypass.Set(bypass ? 0.0f : 1.0f);

    led_mode.Update();
    led_bypass.Update();

    hw.CheckResetToBootloader();
  }

  return 0;
}
