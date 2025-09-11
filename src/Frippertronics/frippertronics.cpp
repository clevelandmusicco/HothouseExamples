// Frippertronics for Hothouse DIY DSP Platform
// Copyright (C) 2024 Jon Philpott <jon.philpott@gmail.com>
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

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisy::System;
using daisysp::DelayLine;
using daisysp::fonepole;
using daisysp::OnePole;
using daisysp::Limiter;
using daisysp::fonepole;
using daisysp::Oscillator;

#define MAX_DELAY static_cast<size_t>(48000 * 23.0f)
#define N_DELAYS (2)

Hothouse hw;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS DELAY_LINES[N_DELAYS];

// grabbed from the Distorion example in Daisy Petal.
float softClip(float in)
{
    if(in > 0)
        return 1 - expf(-in);
    return -1 + expf(in);
}

struct delay_s {
  DelayLine<float, MAX_DELAY> *del;
  float currentDelay;
  float feedback;
  // store the last value of the delay line for the cross-feedback.
  float lastVal;

  // feedback filter lines
  OnePole *fb_lpf, *fb_hpf;

  // LFO for delay time modulation
  Oscillator *lfo;

  float Process(float in, bool erase) {
    del->SetDelay(currentDelay + (lfo->Process() * 48));
    float read = del->Read();
    float actual_fb = erase ? 0.0f : feedback;    
    
    // apply the filters
    read = fb_lpf->Process(fb_hpf->Process(read));
    
    // write back with softclip
    del->Write(daisysp::SoftClip((actual_fb * read) + in));
    lastVal = read;
    return read;
  }
};


// LEDs
Led led_record;
Led led_erase;

bool RECORDING = false;
bool ERASING   = false;

float CROSS_FEEDBACK = 0.0f;
float FEEDBACK       = 0.0f;

delay_s DELAYS[N_DELAYS];
OnePole FB_LPFS[N_DELAYS];
OnePole FB_HPFS[N_DELAYS];
Oscillator LFOS[N_DELAYS];

void InitDelays(float samplerate) {
  for (int i = 0 ; i < N_DELAYS ; i++) {
    DELAY_LINES[i].Init();
    DELAYS[i].del = &DELAY_LINES[i];
    DELAYS[i].currentDelay = samplerate * 1.0f;
    DELAYS[i].feedback = 0.9;
    DELAYS[i].lastVal = 0;

    OnePole fb_lpf;
    OnePole fb_hpf;

    // LPF filter is 8000hz which to my ear is where tape starts to drop
    FB_LPFS[i].Init();
    FB_LPFS[i].SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    FB_LPFS[i].SetFrequency(8000.0f / samplerate);
    
    // HPF filter at 80hz to make sure the resulting audio mash doesnt
    // get super muddy in the sub frequencies.
    FB_HPFS[i].Init();
    FB_HPFS[i].SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
    FB_HPFS[i].SetFrequency(80.0f / samplerate);
    
    DELAYS[i].fb_lpf = &FB_LPFS[i];
    DELAYS[i].fb_hpf = &FB_HPFS[i];

    // differing LFOs for each delay line
    LFOS[i].Init(samplerate);
    LFOS[i].SetFreq(0.05f + (0.03f * i));
    LFOS[i].SetAmp(1.2f + (0.2 * i));
    DELAYS[i].lfo = &LFOS[i];
  }
}

float INPUT_SCALE = 0;
float CURRENT_INPUT_SCALE = 0;
float DELAY_TIME_KNOB = 0;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  RECORDING = hw.switches[Hothouse::FOOTSWITCH_1].Pressed();
  ERASING   = hw.switches[Hothouse::FOOTSWITCH_2].Pressed();

  INPUT_SCALE = RECORDING ? 1.0f : 0.0f;

  // feedback and crossfeedback are ez-here.
  CROSS_FEEDBACK  = hw.knobs[0].Process();
  DELAY_TIME_KNOB = hw.knobs[1].Process();
  FEEDBACK        = hw.knobs[2].Process();

  // fade in the input.
  fonepole(CURRENT_INPUT_SCALE, INPUT_SCALE, 0.02f);

  // bit ol' hack, if the erasing button is held down, set both
  // feedbacks to 0
  if (ERASING) {
    CROSS_FEEDBACK = 0;
    FEEDBACK       = 0;
  }

  DELAYS[0].feedback = DELAYS[1].feedback = FEEDBACK;
  
  for (size_t i = 0; i < size; ++i) {
    // update delays
    float mix = 0;

    // input scale is used here to fade in input to the delay lines
    float in_samp = in[0][i] * CURRENT_INPUT_SCALE;
    
    // manually unrolled the mixing loop
    float sig = DELAYS[0].Process(in_samp + (CROSS_FEEDBACK * DELAYS[1].lastVal), ERASING);
    mix += sig;
    sig = DELAYS[1].Process(in_samp + (CROSS_FEEDBACK * DELAYS[0].lastVal), ERASING);
    mix += sig;

    // reduce the volume of the delay lines
    mix = mix * 0.66f;

    // add the input back in
    mix = mix + in[0][i];

    // out you go.
    out[0][i] = out[1][i] = mix;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  InitDelays(hw.AudioSampleRate());
  
  led_record.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_erase.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    led_record.Set( RECORDING ? 1.0f : 0.0f);
    led_record.Update();

    led_erase.Set( ERASING ? 1.0f : 0.0f);
    led_erase.Update();

    // set the delay times here based on the knob value captured in
    // the audio loop
    if (DELAY_TIME_KNOB < 0.25f) {
      DELAYS[0].currentDelay = 2.503f * hw.AudioSampleRate();
      DELAYS[1].currentDelay = 1.721f * hw.AudioSampleRate();
    } else if (DELAY_TIME_KNOB < 0.50f) {
      DELAYS[0].currentDelay = 5.0f * hw.AudioSampleRate();
      DELAYS[1].currentDelay = 3.0f * hw.AudioSampleRate();
    } else if (DELAY_TIME_KNOB < 0.75f) {
      DELAYS[0].currentDelay = 17.0f * hw.AudioSampleRate();
      DELAYS[1].currentDelay = 11.0f * hw.AudioSampleRate();      
    } else {
      DELAYS[0].currentDelay = 23.0f * hw.AudioSampleRate();
      DELAYS[1].currentDelay = 17.0f * hw.AudioSampleRate();      
    }
  }
  
  return 0;
}
