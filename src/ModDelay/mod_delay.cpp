// ModDelay for Hothouse DIY DSP Platform
// Copyright (C) 2024 Cleveland Music Co. <code@clevelandmusicco.com>
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

// #include "daisy_petal.h"
#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AdcHandle;
using daisy::Parameter;
using daisysp::DelayLine;
using daisysp::fclamp;
using daisysp::fonepole;
using daisysp::Looper;
using daisysp::OnePole;
using daisysp::Oscillator;
using daisysp::Overdrive;
using daisysp::SmoothRandomGenerator;
using daisysp::Svf;

Hothouse hw;
Oscillator osc;
SmoothRandomGenerator srg;
OnePole lpf, hpf;
Svf flt;
Overdrive od;

#define MAX_SIZE (48000 * 60 * 5)  // 5 minutes of floats at 48 khz.
float DSY_SDRAM_BSS buf[MAX_SIZE];
Looper looper;
Oscillator osc_looper_led;  // For pulsing led when recording / paused playback.
// float led_brightness;
int looper_tap_counter;
bool is_looper_double_tap;
bool is_playback_paused;

Parameter mix_param, delay_time_param, delay_fdbk_param, level_param,
    mod_freq_param, mod_depth_param;
Led led_looper, led_bypass;
// bool bypass;

float led_brightness;
bool bypass;
constexpr float kMaxSize = 48000.0f;
constexpr float kLedFreq = 1.0f;
constexpr float kLedInitBrightness = 0.0f;

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delay_line;

struct Delay {
  DelayLine<float, MAX_DELAY> *delay;
  float current_delay, delay_target, feedback;

  float Process(float in) {
    fonepole(current_delay, delay_target, 0.0002f);
    delay->SetDelay(current_delay);
    float read = delay->Read();
    delay->Write((feedback * read) + in);
    return read;
  }
};
Delay delay;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Looper footswitch pressed (start/stop recording, doubletap to pause/unpause
  // playback)
  if (hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
    if (!is_playback_paused) {
      looper.TrigRecord();
      if (!looper.Recording()) {
        // Turn on LED if not recording and in playback
        led_looper.Set(1.f);
      }

      if (!hw.switches[Hothouse::SWITCH_3_UP].Pressed()) {
        looper.SetReverse(true);
      }
    }

    // Start or end double tap timer
    if (is_looper_double_tap) {
      // If second press comes before 1 second, pause playback.
      // 48000Hz/blocksize = 12000 where blocksize is 4.
      if (looper_tap_counter <= 12000) {
        if (looper.Recording()) {
          // Ensure looper is not recording when double tapped (in case it gets
          // double tapped while recording).
          looper.TrigRecord();
        }

        is_playback_paused = !is_playback_paused;

        // Blink LED if paused, otherwise set to triangle wave for pulsing while
        // recording.
        if (is_playback_paused) {
          osc_looper_led.SetWaveform(Oscillator::WAVE_SQUARE);
        } else {
          osc_looper_led.SetWaveform(Oscillator::WAVE_TRI);
        }

        // Reset double tap to prevent weird behaviour when triple clicked.
        looper_tap_counter = 0;
        is_looper_double_tap = false;
        led_looper.Set(1.f);
      }
    } else {
      is_looper_double_tap = true;
    }
  }

  if (is_looper_double_tap) {
    looper_tap_counter += 1;

    if (looper_tap_counter > 12000) {
      // If timer goes beyond 1 second, stop checking for double tap.
      looper_tap_counter = 0;
      is_looper_double_tap = false;
    }
  }

  // If switch1 is held for a second, clear the looper and turn off LED.
  if (hw.switches[Hothouse::FOOTSWITCH_1].TimeHeldMs() >= 1000) {
    is_playback_paused = false;
    osc_looper_led.SetWaveform(Oscillator::WAVE_TRI);  // TODO: needed?
    looper.Clear();
    led_looper.Set(0.f);
  }

  // Toggle effect bypass LED.
  if (hw.switches[Hothouse::FOOTSWITCH_2].FallingEdge()) {
    bypass = !bypass;
    led_bypass.Set(bypass ? 0.f : 1.f);
  }

  for (size_t i = 0; i < size; i++) {
    led_brightness = osc_looper_led.Process();

    if (bypass) {
      // Process looper when in bypass mode
      float loop_out = 0.0f;
      if (!is_playback_paused) {
        loop_out = looper.Process(in[0][i]);
      }

      out[0][i] = in[0][i] + loop_out;
    } else {
      auto mix = mix_param.Process();
      auto delay_time = delay_time_param.Process();
      auto delay_fdbk = delay_fdbk_param.Process();
      auto level = level_param.Process();
      auto mod_freq = mod_freq_param.Process();
      auto mod_depth = mod_depth_param.Process();

      // A nifty near-constant energy crossfade from:
      // https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
      auto x2 = 1.0 - mix;
      auto A = mix * x2;
      auto B = A * (1.0 + 1.4186 * A);
      auto C = B + mix;
      auto D = B + x2;
      auto wet_mix = C * C;
      auto dry_mix = D * D;

      auto calc_delay_target = delay_time * MAX_DELAY;

      // Max mod depth = (MAX_DELAY * 0.0025) = 120 @ 48kHz
      // So, to give room for modulation, set min delay target to
      // 120 and max to 47880 (48000 - 120 = 47880).
      calc_delay_target = fclamp(calc_delay_target, 120, 47880);

      // Only apply modulation if depth knob is set above 1%.
      if (mod_depth > 0.01) {
        mod_freq = fclamp(mod_freq, 0.01, 1);

        if (hw.switches[Hothouse::SWITCH_2_UP].Pressed()) {
          osc.SetFreq(mod_freq * 8);
          osc.SetAmp(mod_depth * 0.0025);
          calc_delay_target = calc_delay_target + (osc.Process() * MAX_DELAY);
        } else {
          // RANDOM modulation from SmoothRandomGenerator.
          srg.SetFreq((mod_freq * 16) + 4);
          calc_delay_target =
              calc_delay_target + (mod_depth * srg.Process() * 120);
        }
      }

      delay.delay_target = calc_delay_target;
      delay.feedback = delay_fdbk;

      float delay_out, lpf_out, hpf_out;

      if (hw.switches[Hothouse::SWITCH_1_UP].Pressed()) {
        // VINTAGE mode; set up like an EHX DMM 7850.
        // https://sites.google.com/site/davidmorrinoldsite/home/trouble/troubleeffects/electro-harmonix-memory-man/eh-7850-calibration
        // Set 3250/60Hz lo/hi pass at min delay time;
        // Decrease LPF to 2.5kHz as delay time increases to 550ms.
        // ~550ms is max on a DMM, so apply the LPF freq change
        // up to that point; keep the freq fixed from 550ms to 1s.
        // 0.55 * 1364 = ~750 (the max desired freq cut).
        auto lpf_freq = fclamp(3250 - (delay_time * 1364), 2500, 3250);
        lpf.SetFrequency(lpf_freq / hw.AudioSampleRate());
        hpf.SetFrequency(60 / hw.AudioSampleRate());

        // Apply filters before delay processing.
        lpf_out = lpf.Process(in[0][i]);
        hpf_out = hpf.Process(lpf_out);
        delay_out = delay.Process(hpf_out);

        // Boost 2.5kHz at min delay time;
        // decrease amplitude to 0 as delay time increases.
        // 0.55 * 1.1 = ~0.6 (to cancel the boost).
        // TODO: try to hit +3dB boost @ min delay time.
        flt.SetFreq(2500);
        flt.Process(delay_out);
        auto amp_adj = (0.6 - fclamp(delay_time * 1.1, 0, 0.6));
        delay_out += flt.Peak() * amp_adj;

        // Add a touch of drive for mojo;
        // increase overdrive as delay time increases.
        od.SetDrive(0.1 + (delay_time * 0.1));

        // Compensate for increasing gain by lowering signal strength
        // as delay time increases. Arbitrary numbers set by ear.
        delay_out = od.Process(delay_out) * (1.6 - (delay_time * 0.6));
      } else {
        // MODERN mode set to 15kHz/20Hz lo/hi pass.
        // Flat EQ; no overdrive.
        lpf.SetFrequency(15000 / hw.AudioSampleRate());
        hpf.SetFrequency(20 / hw.AudioSampleRate());

        lpf_out = lpf.Process(in[0][i]);
        hpf_out = hpf.Process(lpf_out);
        delay_out = delay.Process(hpf_out);
      }

      // Finally, mix with raw input and send to the output buffer.
      auto mix_out = in[0][i] * dry_mix + delay_out * wet_mix;

      // Process Looper
      float loop_out = 0.f;
      if (!is_playback_paused) {
        loop_out = looper.Process(mix_out);
      }

      out[0][i] = (loop_out + mix_out) * (level * 2);
    }
  }

  // Pulse the LED when recording
  if (looper.Recording()) {
    led_looper.Set(led_brightness * 0.5 + 0.5);
  }

  // Blink the LED when paused
  if (is_playback_paused) {
    led_looper.Set(led_brightness * 2.0);
  }

  led_looper.Update();
  led_bypass.Update();
}

void InitHardware() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
}

void InitLooper() {
  looper.Init(buf, kMaxSize);
  looper.SetMode(Looper::Mode::NORMAL);
}

void InitOscillators(float sample_rate) {
  osc_looper_led.Init(sample_rate);
  osc_looper_led.SetFreq(kLedFreq);
  osc_looper_led.SetWaveform(Oscillator::WAVE_TRI);
  led_brightness = kLedInitBrightness;

  osc.Init(sample_rate);
  osc.SetFreq(0.0f);
  osc.SetAmp(0.0f);
}

void InitParameters() {
  mix_param.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
  delay_time_param.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f,
                        Parameter::LINEAR);
  delay_fdbk_param.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f,
                        Parameter::LINEAR);
  level_param.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  mod_freq_param.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f,
                      Parameter::LINEAR);
  mod_depth_param.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f,
                       Parameter::LINEAR);
}

void InitDelay(float sample_rate) {
  delay_line.Init();
  delay.delay = &delay_line;
  delay.delay_target = 0.0f;
  delay.feedback = 0.0f;

  hpf.Init();
  hpf.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
  hpf.SetFrequency(20.0f / sample_rate);

  lpf.Init();
  lpf.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
  lpf.SetFrequency(20000.0f / sample_rate);
}

void InitEffects(float sample_rate) {
  flt.Init(sample_rate);
  od.Init();
  srg.Init(sample_rate);
}

void InitLEDs() {
  led_looper.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_looper.Update();
  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);
  led_bypass.Update();
  bypass = true;
}

int main(void) {
  InitHardware();
  float sample_rate = hw.AudioSampleRate();

  InitLooper();
  InitOscillators(sample_rate);
  InitParameters();
  InitDelay(sample_rate);
  InitEffects(sample_rate);
  InitLEDs();

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    // Do nothing forever to keep the program running ...
  }
}