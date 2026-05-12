// LibreVerb: Freeverb core building blocks.
// Lowpass-feedback comb filter (LBCF) and Schroeder allpass.
//
// Reference: https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
//            https://github.com/sinshu/freeverb
//
// Two changes from stock Freeverb:
//   1. The comb's read tap is fractional and can be modulated, for a
//      Lexicon-style chorused tail.
//   2. A one-pole highpass sits inside the comb's feedback path, after the
//      "damp" lowpass. This bleeds DC and sub-bass out of the recirculating
//      energy so that long decay times don't ring up runaway low rumble.
//      The HIGHS knob controls the lowpass, the LOWS knob controls the
//      highpass.

#pragma once

#include <cstddef>
#include <cstdint>

namespace libreverb {

// Power-of-two buffer sizes so we can wrap with bit masks (one cycle on
// Cortex-M7) instead of integer modulo (slower, branchy). The masks live
// in the buffer-size constants below.
constexpr size_t kCombBufSize = 4096;  // mask = 0x0FFF
constexpr size_t kCombBufMask = kCombBufSize - 1;
constexpr size_t kApBufSize = 1024;  // mask = 0x03FF
constexpr size_t kApBufMask = kApBufSize - 1;

// Stock Freeverb tunings (samples at 44.1 kHz). We scale these at runtime
// for the current sample rate and for the SIZE switch (Plate/Hall/Cathedral).
constexpr int kNumCombs = 8;
constexpr int kNumAllpasses = 4;

// File-scope arrays (not struct members) to keep the toolchain happy.
// See repo notes: arm-none-eabi-g++ -std=gnu++14 has trouble with
// static-constexpr-array members of structs (ODR linker errors).
constexpr int kCombLensL[kNumCombs] = {1116, 1188, 1277, 1356,
                                       1422, 1491, 1557, 1617};
constexpr int kApLensL[kNumAllpasses] = {556, 441, 341, 225};

// Stereo spread: right-channel delays are L+spread samples. Different
// delay lengths on L vs R is the entire reason a Freeverb fed mono input
// produces a true-stereo output (decorrelated tails, not just panned).
constexpr int kStereoSpread = 23;

// Lowpass-feedback comb with HIGHS damp, LOWS damp, and fractional read.
// All state is plain floats; the buffer is owned by the caller and lives
// in SDRAM (the Daisy Seed only has ~512KB of fast internal SRAM).
class Comb {
 public:
  void Init(float* buf) {
    buf_ = buf;
    writepos_ = 0;
    feedback_ = 0.5f;
    damp_ = 0.4f;
    damp_inv_ = 0.6f;
    lp_state_ = 0.0f;
    hp_a_ = 0.0f;
    hp_state_ = 0.0f;
    base_delay_ = 1.0f;
    mod_offset_ = 0.0f;
    Clear();
  }

  void Clear() {
    for (size_t i = 0; i < kCombBufSize; ++i) buf_[i] = 0.0f;
    lp_state_ = 0.0f;
    hp_state_ = 0.0f;
  }

  inline void SetBaseDelay(float samples) { base_delay_ = samples; }
  inline void SetFeedback(float fb) { feedback_ = fb; }
  inline void SetDamp(float d) {
    damp_ = d;
    damp_inv_ = 1.0f - d;
  }
  inline void SetLowDamp(float a) { hp_a_ = a; }
  inline void SetModOffset(float off) { mod_offset_ = off; }

  // One sample in, one sample out (the comb's contribution to the wet bus).
  // The damping LPF and HPF are INSIDE the feedback loop, so each successive
  // bounce around the comb gets a little darker and a little less bass --
  // that's how the tail's spectrum evolves over time.
  inline float Process(float in) {
    // Fractional read at (base + LFO offset) samples behind write head.
    float tap = base_delay_ + mod_offset_;
    if (tap < 1.0f) tap = 1.0f;
    if (tap > static_cast<float>(kCombBufSize) - 2.0f)
      tap = static_cast<float>(kCombBufSize) - 2.0f;

    const int32_t tap_i = static_cast<int32_t>(tap);
    const float tap_f = tap - static_cast<float>(tap_i);

    const size_t i0 = (writepos_ - tap_i) & kCombBufMask;
    const size_t i1 = (writepos_ - tap_i - 1) & kCombBufMask;
    const float a = buf_[i0];
    const float b = buf_[i1];
    const float read = a + (b - a) * tap_f;

    // HIGHS damp: one-pole LPF on the feedback signal.
    lp_state_ = read * damp_inv_ + lp_state_ * damp_;

    // LOWS damp: one-pole HPF after the LPF. State tracks the slow drift;
    // output = signal minus that drift.
    hp_state_ += hp_a_ * (lp_state_ - hp_state_);
    const float fb_signal = lp_state_ - hp_state_;

    buf_[writepos_] = in + fb_signal * feedback_;
    writepos_ = (writepos_ + 1) & kCombBufMask;
    return read;
  }

 private:
  float* buf_ = nullptr;
  size_t writepos_ = 0;
  float feedback_ = 0.5f;
  float damp_ = 0.4f;  // LPF coefficient: 0=bright, ~0.95=very dark
  float damp_inv_ = 0.6f;
  float lp_state_ = 0.0f;
  float hp_a_ = 0.0f;  // HPF coefficient: 0=full bass, ~0.05=~400Hz cut
  float hp_state_ = 0.0f;
  float base_delay_ = 1.0f;
  float mod_offset_ = 0.0f;
};

// Stock Schroeder allpass, g = 0.5. No modulation -- the allpasses are
// here to smear comb output into diffuse reflections, not to wobble.
class Allpass {
 public:
  void Init(float* buf) {
    buf_ = buf;
    writepos_ = 0;
    delay_ = 1;
    Clear();
  }

  void Clear() {
    for (size_t i = 0; i < kApBufSize; ++i) buf_[i] = 0.0f;
  }

  inline void SetDelay(int d) {
    if (d < 1) d = 1;
    if (static_cast<size_t>(d) >= kApBufSize) d = kApBufSize - 1;
    delay_ = d;
  }

  inline float Process(float in) {
    const size_t r = (writepos_ - delay_) & kApBufMask;
    const float bufout = buf_[r];
    const float out = -in + bufout;
    // g = 0.5 hardcoded, the stock Freeverb value.
    buf_[writepos_] = in + bufout * 0.5f;
    writepos_ = (writepos_ + 1) & kApBufMask;
    return out;
  }

 private:
  float* buf_ = nullptr;
  size_t writepos_ = 0;
  int delay_ = 1;
};

}  // namespace libreverb
