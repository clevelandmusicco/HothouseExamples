#pragma once
#ifndef CMC_EXT_OSCILLATOR_H
#define CMC_EXT_OSCILLATOR_H
#include <stdint.h>
#ifdef __cplusplus

namespace clevelandmusicco {
/** Synthesis of several waveforms, including polyBLEP bandlimited waveforms.
 */
class FlickOscillator {
 public:
  FlickOscillator() {}
  ~FlickOscillator() {}
  /** Choices for output waveforms, POLYBLEP are appropriately labeled. Others
   * are naive forms.
   */
  enum {
    WAVE_SIN,
    WAVE_TRI,
    WAVE_SAW,
    WAVE_RAMP,
    WAVE_SQUARE,
    WAVE_POLYBLEP_TRI,
    WAVE_POLYBLEP_SAW,
    WAVE_POLYBLEP_SQUARE,
    WAVE_SQUARE_ROUNDED,
    WAVE_LAST,
  };

  /** quick fp clamp
   */
  inline float fclamp(float in, float min, float max) {
    return fmin(fmax(in, min), max);
  }

  /** Significantly more efficient than fmodf(x, 1.0f) for calculating
   *  the decimal part of a floating point value.
   */
  inline float fastmod1f(float x) { return x - static_cast<int>(x); }

  /** Initializes the Oscillator

      \param sample_rate - sample rate of the audio engine being run, and the
     frequency that the Process function will be called.

      Defaults:
      - freq_ = 100 Hz
      - amp_ = 0.5
      - waveform_ = sine wave.
  */
  void Init(float sample_rate) {
    sr_ = sample_rate;
    sr_recip_ = 1.0f / sample_rate;
    freq_ = 100.0f;
    amp_ = 0.5f;
    pw_ = 0.5f;
    phase_ = 0.0f;
    phase_inc_ = CalcPhaseInc(freq_);
    waveform_ = WAVE_SIN;
    eoc_ = true;
    eor_ = true;
  }

  /** Gets the current frequency of the oscillator.
   */
  inline float GetFreq() { return freq_; }
  
  /** Changes the frequency of the Oscillator, and recalculates phase
   * increment.
   */
  inline void SetFreq(const float f) {
    freq_ = f;
    phase_inc_ = CalcPhaseInc(f);
  }
  
  /** Gets the amplitude of the oscillator.
   */
  inline float GetAmp() { return amp_; }
  
  /** Sets the amplitude of the waveform.
   */
  inline void SetAmp(const float a) { amp_ = a; }
  
  /** Sets the waveform to be synthesized by the Process() function.
   */
  inline void SetWaveform(const uint8_t wf) {
    waveform_ = wf < WAVE_LAST ? wf : WAVE_SIN;
  }

  /** Sets the pulse width for WAVE_SQUARE and WAVE_POLYBLEP_SQUARE (range 0 -
   * 1)
   */
  inline void SetPw(const float pw) { pw_ = fclamp(pw, 0.0f, 1.0f); }

  /** Returns true if cycle is at end of rise. Set during call to Process.
   */
  inline bool IsEOR() { return eor_; }

  /** Returns true if cycle is at end of cycle. Set during call to Process.
   */
  inline bool IsEOC() { return eoc_; }

  /** Returns true if cycle rising.
   */
  inline bool IsRising() { return phase_ < 0.5f; }

  /** Returns true if cycle falling.
   */
  inline bool IsFalling() { return phase_ >= 0.5f; }

  /** Processes the waveform to be generated, returning one sample. This
   * should be called once per sample period.
   */
  float Process();

  /** Adds a value 0.0-1.0 (equivalent to 0.0-TWO_PI) to the current phase.
   * Useful for PM and "FM" synthesis.
   */
  void PhaseAdd(float _phase) { phase_ += _phase; }
  /** Resets the phase to the input argument. If no argumeNt is present, it
   * will reset phase to 0.0;
   */
  void Reset(float _phase = 0.0f) { phase_ = _phase; }

 private:
  float CalcPhaseInc(float f);
  uint8_t waveform_;
  float amp_, freq_, pw_;
  float sr_, sr_recip_, phase_, phase_inc_;
  float last_out_, last_freq_;
  bool eor_, eoc_;
};
}  // namespace clevelandmusicco
#endif
#endif
