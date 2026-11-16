// Interplay for Hothouse DIY DSP Platform
// Copyright (C) 2026 Jon Philpott <jon.philpott@gmail.com>
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

// ### Uncomment if IntelliSense can't resolve DaisySP-LGPL classes ###
// #include "daisysp-lgpl.h"

/*
 * Hothouse version of Interplay, an evolution of the frippertronics
 * effect.
 */

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AnalogControl;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisy::Switch;
using daisy::System;
using daisysp::DelayLine;
using daisysp::fonepole;
using daisysp::Limiter;
using daisysp::OnePole;
using daisysp::Oscillator;
using daisysp::WhiteNoise;

// delay timing parameters
#define GOLDEN_RATIO (1.6180339f)  // used for delay time ratio between channels
#define MIN_DELAY_TIME (0.8f)      // minimum delay time in seconds
#define MAX_DELAY_TIME (30.0f)     // maximum delay time in seconds
#define DELAY_BUFFER_MULTIPLIER \
  (1.2f)  // Buffer size multiplier for LFO modulation headroom

// maximum delay line duration in seconds
#define MAX_DELAY_LINE_TIME \
  ((MAX_DELAY_TIME) * GOLDEN_RATIO * DELAY_BUFFER_MULTIPLIER)
#define MAX_DELAY static_cast<size_t>(48000 * (MAX_DELAY_LINE_TIME))

#define N_DELAYS (2)

// delay time setting is smoothed for tape woop-woop effect
#define DELAY_SMOOTHING_COEFF (0.00002f)

// noise changes based on delay-time
#define TAPE_HPF_MULTIPLIER (0.125f)

#define INITIAL_FEEDBACK (0.9f)  // Initial feedback coefficient for delay lines

// allow feedback gains to go higher than unity to offset for gain
// loss from the filters
#define FEEDBACK_MULTIPLIER (1.2f)

// Filter frequency parameters
#define FEEDBACK_LPF_FREQ (8000.0f)      // LPF cutoff for tape effect
#define FEEDBACK_HPF_FREQ (60.0f)        // HPF cutoff for tape effect
#define TAPE_HPF_CENTER_FREQ (12000.0f)  // center for noise

// LFO parameters

#define LFO_LFO_BASE_FREQ (0.0005f)       // Base LFO frequency (Hz)
#define LFO_LFO_FREQ_INCREMENT (0.0003f)  // offset between delay LFOs
#define LFO_LFO_BASE_AMP (0.002f)         // Base LFO amplitude multiplier
#define LFO_LFO_AMP_INCREMENT (0.002f)    // Per-delay LFO amplitude increment

#define LFO_BASE_FREQ (0.005f)       // Base LFO frequency (Hz)
#define LFO_FREQ_INCREMENT (0.003f)  // Per-delay LFO frequency increment (Hz)
#define LFO_BASE_AMP (0.10f)         // Base LFO amplitude multiplier
#define LFO_AMP_INCREMENT (0.1f)     // Per-delay LFO amplitude increment
#define TAPE_LFO_AMP (4000.0f)       // Tape HPF LFO amplitude (Hz)
#define TAPE_LFO_FREQ (2.0f)         // Tape HPF LFO frequency (Hz)

#define NOISE_AMP_MAX (0.001f)  // max white noise amplitude for tape effect
#define INPUT_SCALE_SMOOTHING (0.002f)  // fade-in for input
#define MIX_OUTPUT_SCALE (0.66f)        // Output gain scaling for delay mix

#define DELAY_TIME_BUCKETS (128.0f)  // Number of delay time resolution buckets
#define AUDIO_BLOCK_SIZE (4)         // Audio samples per callback

#define KNOB_SMOOTHING (0.05f)  // Knob slew time in seconds

// Erase state machine parameters
#define ERASE_STATE_WAIT 0  // Erase button state: waiting for first press
#define ERASE_STATE_WAIT_RELEASE 1  // Erase button state: waiting for release
#define ERASE_STATE_WAIT_PRESS_AGAIN \
  2                             // Erase button state: waiting for second press
#define ERASE_TIMEOUT_MS (800)  // Timeout in milliseconds to reset erase state

Hothouse hw;

DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS DELAY_LINES[N_DELAYS];

struct delay_s {
  DelayLine<float, MAX_DELAY>* del;
  float currentDelay;
  float delayTarget;
  float feedback;
  float lastVal;
  float samplerate;
  float lfo_freq;

  WhiteNoise noise;
  OnePole fb_lpf, fb_hpf;
  OnePole tape_hpf;

  Oscillator lfo_lfo;
  Oscillator lfo;
  Oscillator tape_hpf_lfo;
  float tape_hpf_cf;

  void Init(DelayLine<float, MAX_DELAY>* line, float sr, int index) {
    del = line;
    del->Init();

    currentDelay = sr * 1.0f;
    delayTarget = sr * 1.0f;
    feedback = INITIAL_FEEDBACK;
    lastVal = 0;
    samplerate = sr;
    tape_hpf_cf = TAPE_HPF_CENTER_FREQ;

    // init noise
    noise.Init();
    noise.SetAmp(0);

    // init feedback filters
    // LPF filter is 8000hz which to my ear is where tape starts to drop
    fb_lpf.Init();
    fb_lpf.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    fb_lpf.SetFrequency(FEEDBACK_LPF_FREQ / sr);

    // HPF filter at 60hz to make sure the resulting audio mash doesnt
    // get super muddy in the sub frequencies.
    fb_hpf.Init();
    fb_hpf.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
    fb_hpf.SetFrequency(FEEDBACK_HPF_FREQ / sr);

    // init tape HPF
    tape_hpf.Init();
    tape_hpf.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);

    // LFO of LFOs
    lfo_lfo.Init(sr);
    lfo_lfo.SetFreq(LFO_LFO_BASE_FREQ + (LFO_LFO_FREQ_INCREMENT * index));
    lfo_lfo.SetAmp(LFO_LFO_BASE_AMP + (LFO_LFO_AMP_INCREMENT * index));

    // init delay time LFO
    lfo.Init(sr);
    lfo_freq = LFO_BASE_FREQ + (LFO_FREQ_INCREMENT * index);
    lfo.SetFreq(lfo_freq);

    // lfo is 100ms max (plus some variation from the samplerate,
    // we'll scale these for each delay time in the delay loop
    lfo.SetAmp((LFO_BASE_AMP + (LFO_AMP_INCREMENT * index)) * sr);

    // init tape HPF LFO
    tape_hpf_lfo.Init(sr);
    tape_hpf_lfo.SetAmp(TAPE_LFO_AMP);
    tape_hpf_lfo.SetFreq(TAPE_LFO_FREQ);
  }

  float Process(float in) {
    fonepole(currentDelay, delayTarget, DELAY_SMOOTHING_COEFF);

    float delay_fraction = currentDelay / (samplerate * MAX_DELAY_TIME);
    lfo.SetFreq(lfo_freq + lfo_lfo.Process());
    float lfo_val = lfo.Process();
    del->SetDelay(currentDelay + (lfo_val * delay_fraction));
    float read = del->Read();

    // apply the filters
    read = fb_lpf.Process(fb_hpf.Process(read));

    // write back with softclip
    del->Write(daisysp::SoftClip((feedback * read) + in));

    // add some noise, use delay time to move the LFO tape HPF
    // cutoff around. apply after we write to the delay so the
    // hiss doesn't grow unless cross-feedback is enabled
    tape_hpf_lfo.SetFreq(1.0f / ((1.0f + currentDelay) * TAPE_HPF_MULTIPLIER));
    tape_hpf.SetFrequency((tape_hpf_cf + tape_hpf_lfo.Process()) / samplerate);
    read = read + tape_hpf.Process(noise.Process());

    lastVal = read;
    return read;
  }
};

// LEDs
Led led_record;
Led led_erase;

bool RECORDING = false;
bool ERASING = false;
bool STEREO = false;

float CROSS_FEEDBACK = 0.0f;
float FEEDBACK = 0.0f;
float WET_DRY = 0.0f;
float NOISE_AMP = 0.0f;

float LEFT_VOL = 0.0f;
float RIGHT_VOL = 0.0f;

delay_s DELAYS[N_DELAYS];

float INPUT_SCALE = 0;
float CURRENT_INPUT_SCALE = 0;
float DELAY_TIME_KNOB = 0;
uint32_t FRAME_COUNTER = 0;

uint32_t ERASE_STATE = ERASE_STATE_WAIT;
uint32_t ERASE_PRESSED_WHEN = 0;

Limiter limiter_left;
Limiter limiter_right;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // SW_ACTIVE() hides the per-board electrical sense (see BOARD SELECT above).
  RECORDING = hw.switches[Hothouse::FOOTSWITCH_1].Pressed();
  ERASING = hw.switches[Hothouse::FOOTSWITCH_2].Pressed();

  INPUT_SCALE = RECORDING ? 1.0f : 0.0f;

  CROSS_FEEDBACK = hw.knobs[0].Process() * FEEDBACK_MULTIPLIER;
  DELAY_TIME_KNOB = hw.knobs[1].Process();
  FEEDBACK = hw.knobs[2].Process() * FEEDBACK_MULTIPLIER;
  WET_DRY = hw.knobs[4].Process();
  NOISE_AMP = hw.knobs[3].Process();

  Hothouse::ToggleswitchPosition sw1_pos =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
  Hothouse::ToggleswitchPosition sw2_pos =
      hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);

  STEREO = (sw1_pos == Hothouse::TOGGLESWITCH_UP);

  switch (sw2_pos) {
    case Hothouse::TOGGLESWITCH_UP:
      LEFT_VOL = 1.0f;
      RIGHT_VOL = 0.0f;
      break;
    case Hothouse::TOGGLESWITCH_UNKNOWN:
    case Hothouse::TOGGLESWITCH_MIDDLE:
      LEFT_VOL = 1.0f;
      RIGHT_VOL = 1.0f;
      break;
    case Hothouse::TOGGLESWITCH_DOWN:
      LEFT_VOL = 0.0f;
      RIGHT_VOL = 1.0f;
      break;
  }

  // Double tap erase statemachine stuff.
  uint32_t now = System::GetNow();
  uint32_t tdiff = now - ERASE_PRESSED_WHEN;

  // timeout after 2 seconds
  if (ERASE_STATE > ERASE_STATE_WAIT && tdiff > ERASE_TIMEOUT_MS) {
    ERASE_STATE = ERASE_STATE_WAIT;
  }

  if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
    if (ERASE_STATE == ERASE_STATE_WAIT) {
      ERASE_PRESSED_WHEN = now;
      ERASE_STATE = ERASE_STATE_WAIT_RELEASE;
    }

    if (ERASE_STATE == ERASE_STATE_WAIT_PRESS_AGAIN) {
      DELAYS[0].del->Reset();
      DELAYS[1].del->Reset();

      // disable feedbacks for this cycle, this prevents a "click"
      // from entering the loop
      ERASING = true;
      ERASE_STATE = ERASE_STATE_WAIT;
    }
  }

  // advance state if the initial press was released.
  if (hw.switches[Hothouse::FOOTSWITCH_2].FallingEdge()) {
    if (ERASE_STATE == ERASE_STATE_WAIT_RELEASE) {
      ERASE_STATE = ERASE_STATE_WAIT_PRESS_AGAIN;
    }
  }

  // bit ol' hack, if the erasing button is held down, set both
  // feedbacks to 0, it works.
  if (ERASING) {
    CROSS_FEEDBACK = 0;
    FEEDBACK = 0;
  }

  DELAYS[0].feedback = DELAYS[1].feedback = FEEDBACK;
  DELAYS[0].noise.SetAmp(NOISE_AMP * NOISE_AMP_MAX);
  DELAYS[1].noise.SetAmp(NOISE_AMP * NOISE_AMP_MAX);

  for (size_t i = 0; i < size; ++i) {
    // float delays_out = 0.0f;
    float left_out = 0.0f;
    float right_out = 0.0f;

    float mix = 0;
    // fade in the input, so new audio comes in smooth and not with a big loud
    // click.. i'm looking at you, other loopers.
    fonepole(CURRENT_INPUT_SCALE, INPUT_SCALE, INPUT_SCALE_SMOOTHING);

    float in_samp = in[0][i];
    float in_samp_fade = in_samp * CURRENT_INPUT_SCALE;

    // each delay gets the dry input plus a scaled tap of the
    // *other* delay's most recent output. i suspect the compiler
    // would have done this anyway.
    float left_delay_out =
        DELAYS[0].Process(in_samp_fade + (CROSS_FEEDBACK * DELAYS[1].lastVal)) *
        LEFT_VOL;
    float right_delay_out =
        DELAYS[1].Process(in_samp_fade + (CROSS_FEEDBACK * DELAYS[0].lastVal)) *
        RIGHT_VOL;

    float dry_vol = 1.0f - WET_DRY;
    float wet_vol = WET_DRY;

    if (STEREO) {
      left_out =
          (in_samp * dry_vol) + (left_delay_out * MIX_OUTPUT_SCALE * wet_vol);
      right_out =
          (in_samp * dry_vol) + (right_delay_out * MIX_OUTPUT_SCALE * wet_vol);
    } else {
      left_out = right_out =
          (in_samp * dry_vol) +
          ((left_delay_out + right_delay_out) * MIX_OUTPUT_SCALE * wet_vol);
    }

    limiter_left.ProcessBlock(&left_out, 1, 1.0f);
    limiter_right.ProcessBlock(&right_out, 1, 1.0f);

    out[0][i] = left_out;
    out[1][i] = right_out;
  }
}

int main() {
  // HotHouse hardware initialisation
  hw.Init();
  hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  led_record.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_erase.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  for (int i = 0; i < N_DELAYS; i++) {
    DELAYS[i].Init(&DELAY_LINES[i], hw.AudioSampleRate(), i);
  }

  limiter_left.Init();
  limiter_right.Init();

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    led_record.Set(RECORDING ? 1.0f : 0.0f);
    led_record.Update();

    led_erase.Set(ERASING ? 1.0f : 0.0f);
    led_erase.Update();

    // this guy is a little tricky to read, but basically buckets
    // the delay time knob into 128 slices.
    int delay_time_bucket = (int)round(DELAY_TIME_KNOB * DELAY_TIME_BUCKETS);

    float base_delay_time =
        (MIN_DELAY_TIME +
         ((MAX_DELAY_TIME - MIN_DELAY_TIME) * (delay_time_bucket / 128.0f))) *
        hw.AudioSampleRate();

    DELAYS[0].delayTarget = base_delay_time;
    // set the delay time based off of the golden ratio, this is the
    // estimated co-primish magic.
    DELAYS[1].delayTarget = base_delay_time * GOLDEN_RATIO;

    // Hold *BOTH* footswitches for 2 s to reset to bootloader
    hw.CheckResetToBootloader();
  }

  return 0;
}
