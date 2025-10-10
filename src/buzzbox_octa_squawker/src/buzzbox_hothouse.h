// buzzbox_hothouse.h
// Buzzbox Octa Squawker - Aggressive Fuzz with Envelope Filter and Octave
// Phase 8 - Complete control redesign with context-dependent knobs
// FIX: Added anti-aliasing filter for octave processing

#pragma once
#ifndef BUZZBOX_HOTHOUSE_H
#define BUZZBOX_HOTHOUSE_H

#include <vector>
#include <cmath>
#include <algorithm>

// =============================================================================
// FUZZ MODULE - Aggressive Type Only
// =============================================================================

enum class FuzzType {
    CLEAN = 0,      // No gate, natural decay (not used in Phase 8)
    GATED = 1,      // Aggressive gate, staccato (not used in Phase 8)
    AGGRESSIVE = 2  // Asymmetric clip, more harmonics, pre-emphasis (default)
};

namespace Fuzz {
    inline float softClipping(float input, float gain) {
        return std::tanh(input * gain);
    }
    
    inline float asymmetricClip(float input, float intensity) {
        const float pos_threshold = 0.8f;
        const float neg_threshold = 1.0f;
        
        if (input > 0.0f) {
            return softClipping(input, intensity) * pos_threshold;
        } else {
            return softClipping(input, intensity) * neg_threshold;
        }
    }
    
    inline float fuzzEffect(float input, float intensity, FuzzType type) {
        static float dc_blocker_x1 = 0.0f;
        static float dc_blocker_y1 = 0.0f;
        static float gate_envelope = 0.0f;
        static float pre_emphasis_state = 0.0f;
        
        float fuzzed = input;
        
        if (type == FuzzType::AGGRESSIVE) {
            const float emphasis_coeff = 0.7f;
            float pre_emphasized = input + (input - pre_emphasis_state) * emphasis_coeff;
            pre_emphasis_state = input;
            fuzzed = pre_emphasized;
        }
        
        if (type == FuzzType::AGGRESSIVE) {
            fuzzed = asymmetricClip(fuzzed, intensity);
        } else {
            fuzzed = softClipping(fuzzed, intensity);
        }
        
        if (type == FuzzType::AGGRESSIVE) {
            fuzzed += 0.03f * (input * input);
            fuzzed += 0.012f * (input * input * input * input);
            fuzzed += 0.015f * (input * input * input);
        } else {
            fuzzed += 0.02f * (input * input) + 0.008f * (input * input * input * input);
        }
        
        const float dynamicIntensity = intensity * (1.0f + 0.5f * std::abs(input));
        fuzzed = softClipping(fuzzed, dynamicIntensity);
        
        const float dc_coeff = 0.997f;
        float dc_blocked = fuzzed - dc_blocker_x1 + dc_coeff * dc_blocker_y1;
        dc_blocker_x1 = fuzzed;
        dc_blocker_y1 = dc_blocked;
        
        if (type == FuzzType::AGGRESSIVE) {
            static float de_emphasis_state = 0.0f;
            const float de_emphasis_coeff = 0.3f;
            de_emphasis_state = de_emphasis_state + de_emphasis_coeff * (dc_blocked - de_emphasis_state);
            dc_blocked = de_emphasis_state;
        }
        
        if (type == FuzzType::CLEAN) {
            return dc_blocked;
        } else {
            // Note: Gate is now controlled externally via Knob 6
            // This internal gate is kept for non-AGGRESSIVE types
            const float gate_threshold = 0.03f;
            const float gate_attack = 0.95f;
            const float gate_release = 0.01f;
            
            float input_level = std::abs(input);
            
            if (input_level > gate_threshold) {
                gate_envelope += gate_attack * (1.0f - gate_envelope);
            } else {
                gate_envelope += gate_release * (0.0f - gate_envelope);
            }
            
            return dc_blocked * gate_envelope;
        }
    }
    
    inline float process(float sample, FuzzType type, float intensity) {
        return fuzzEffect(sample, intensity * 10.0f, type);
    }
}

// =============================================================================
// ENVELOPE FOLLOWER
// =============================================================================

class EnvelopeFollower {
public:
    EnvelopeFollower() : envelope_level_(0.0f) {}
    
    void Init(float samplerate, float attack_ms, float release_ms) {
        samplerate_ = samplerate;
        setAttackRelease(attack_ms, release_ms);
    }
    
    void setAttackRelease(float attack_ms, float release_ms) {
        attack_coeff_ = 1.0f - std::exp(-1.0f / (attack_ms * samplerate_ / 1000.0f));
        release_coeff_ = 1.0f - std::exp(-1.0f / (release_ms * samplerate_ / 1000.0f));
    }
    
    float Process(float input) {
        float input_level = std::abs(input);
        
        if (input_level > envelope_level_) {
            envelope_level_ += attack_coeff_ * (input_level - envelope_level_);
        } else {
            envelope_level_ += release_coeff_ * (input_level - envelope_level_);
        }
        
        return envelope_level_;
    }
    
    float GetEnvelopeLevel() const { return envelope_level_; }
    void Reset() { envelope_level_ = 0.0f; }

private:
    float samplerate_;
    float envelope_level_;
    float attack_coeff_;
    float release_coeff_;
};

// =============================================================================
// ANTI-ALIASING FILTER FOR OCTAVE PROCESSING
// =============================================================================

class AntiAliasingFilter {
public:
    AntiAliasingFilter() : x1_(0.0f), y1_(0.0f), coeff_(0.0f) {}
    
    void Init(float samplerate, float cutoff_freq) {
        // Simple one-pole lowpass: y[n] = a * x[n] + (1-a) * y[n-1]
        // cutoff = samplerate / (2 * pi * RC), solve for coefficient
        float rc = 1.0f / (2.0f * M_PI * cutoff_freq);
        float dt = 1.0f / samplerate;
        coeff_ = dt / (rc + dt);
    }
    
    float Process(float input) {
        // One-pole lowpass filter
        y1_ = coeff_ * input + (1.0f - coeff_) * y1_;
        return y1_;
    }
    
    void Reset() {
        x1_ = 0.0f;
        y1_ = 0.0f;
    }

private:
    float coeff_;
    float x1_;
    float y1_;
};

// =============================================================================
// OVERSAMPLING SYSTEM - Reduced to 4x for Performance
// =============================================================================

constexpr int OVERSAMPLING_FACTOR = 4;

namespace Oversampling {
    inline std::vector<float> upsample(float input) {
        std::vector<float> output(OVERSAMPLING_FACTOR, 0.0f);
        output[0] = input;
        for (int i = 1; i < OVERSAMPLING_FACTOR; ++i) {
            output[i] = input * (1.0f - static_cast<float>(i) / OVERSAMPLING_FACTOR);
        }
        return output;
    }
    
    inline float downsample(const std::vector<float>& input) {
        float sum = 0.0f;
        for (float sample : input) {
            sum += sample;
        }
        return sum / input.size();
    }
}

// =============================================================================
// PARAMETER RANGES
// =============================================================================

struct ParameterRanges {
    static constexpr float GAIN_MIN = 1.0f;
    static constexpr float GAIN_MAX = 20.0f;
    static constexpr float TONE_MIN = 0.0f;
    static constexpr float TONE_MAX = 1.0f;
    static constexpr float LEVEL_MIN = 0.0f;
    static constexpr float LEVEL_MAX = 1.0f;
    static constexpr float INTENSITY_MIN = 0.0f;
    static constexpr float INTENSITY_MAX = 1.0f;
    static constexpr float MIX_MIN = 0.0f;
    static constexpr float MIX_MAX = 1.0f;
    static constexpr float OCTAVE_MIN = 0.0f;
    static constexpr float OCTAVE_MAX = 1.0f;
    
    static constexpr float TONE_FREQ_MIN = 100.0f;
    static constexpr float TONE_FREQ_MAX = 1500.0f;
    
    // Input gain range (Knob 1)
    static constexpr float INPUT_GAIN_MIN = 0.5f;
    static constexpr float INPUT_GAIN_MAX = 2.0f;
    
    // Autowah filter range
    static constexpr float AUTOWAH_FREQ_MIN = 100.0f;
    static constexpr float AUTOWAH_FREQ_MAX = 3000.0f;
};

#endif // BUZZBOX_HOTHOUSE_H
