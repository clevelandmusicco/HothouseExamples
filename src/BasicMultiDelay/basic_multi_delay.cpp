// BasicMultiDelay for Hothouse DIY DSP Platform
// Copyright (C) 2024 Your Name <your@email>
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

// Ported directly from the 'petal/MultiDelay' example in DaisyExamples. Knobs,
// switches, and pins are directly accessed without enums, so look at hothouse.h
// to decipher the mappings.

// It's fair to call this code 'obfuscated'; it's been left (mostly) as-is.

#include "daisysp.h"
#include "hothouse.h"

#define MAX_DELAY static_cast<size_t>(48000 * 1.0f)

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::SaiHandle;
using daisy::System;
using daisysp::DelayLine;
using daisysp::fonepole;

Hothouse hw;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delMems[3];

struct delay {
  DelayLine<float, MAX_DELAY> *del;
  float currentDelay;
  float delayTarget;
  float feedback;

  float Process(float in) {
    // set delay times
    fonepole(currentDelay, delayTarget, 0.0002f);
    del->SetDelay(currentDelay);

    float read = del->Read();
    del->Write((feedback * read) + in);

    return read;
  }
};

delay delays[3];
Parameter params[3];

float feedback, volume;
int drywet;

// Bypass vars
Led led_bypass;
bool bypass = true;

void InitDelays(float samplerate) {
  for (int i = 0; i < 3; i++) {
    // Init delays
    delMems[i].Init();
    delays[i].del = &delMems[i];
    // 3 delay times
    params[i].Init(hw.knobs[i], samplerate * 0.05, MAX_DELAY,
                   Parameter::LOGARITHMIC);
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // knobs
  for (int i = 0; i < 3; i++) {
    delays[i].delayTarget = params[i].Process();
    delays[i].feedback = hw.knobs[(i) + 3].Process();
  }

  // we're out of knobs; use a toggleswitch with predefined drywet values
  static const int drywetValues[] = {100, 50, 33};
  drywet = drywetValues[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)];

  // some settings result in low volume; use a toggleswitch to select from
  // predefined volume multipliers
  static const float volumeValues[] = {2.0f, 1.5f, 1.0f};
  volume = volumeValues[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)];

  // footswitch
  bypass ^= hw.switches[7].RisingEdge();

  for (size_t i = 0; i < size; ++i) {
    float mix = 0;
    float fdrywet = bypass ? 0.0f : (float)drywet / 100.f;

    // update delayline with feedback
    for (int d = 0; d < 3; d++) {
      float sig = delays[d].Process(in[0][i]);
      mix += sig;
    }

    // apply drywet and attenuate
    mix = fdrywet * mix * 0.333f + (1.0f - fdrywet) * in[0][i];

    // Apply overall output volume control
    if (!bypass) {
      mix *= volume;
    }

    out[0][i] = out[1][i] = mix;
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(4);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  InitDelays(hw.AudioSampleRate());

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();
    System::Delay(10);
    hw.CheckResetToBootloader();
  }
  return 0;
}