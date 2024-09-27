// HardwareTest for Hothouse DIY DSP Platform
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

#include "daisysp.h"
#include "hothouse.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using daisysp::Oscillator;

Hothouse hw;

// Define the number of oscillators
const int NUM_OSCILLATORS = 3;

// Oscillator array
Oscillator oscillators[NUM_OSCILLATORS];

// Array for frequency and amplitude knobs corresponding to each oscillator
const Hothouse::Knob freq_knobs[NUM_OSCILLATORS] = {
    Hothouse::KNOB_1, Hothouse::KNOB_2, Hothouse::KNOB_3};
const Hothouse::Knob amp_knobs[NUM_OSCILLATORS] = {
    Hothouse::KNOB_4, Hothouse::KNOB_5, Hothouse::KNOB_6};

// Array for toggle switches corresponding to each oscillator
const Hothouse::Toggleswitch toggle_switches[NUM_OSCILLATORS] = {
    Hothouse::TOGGLESWITCH_1, Hothouse::TOGGLESWITCH_2,
    Hothouse::TOGGLESWITCH_3};

// Array of waveforms corresponding to toggle switch positions
const int waveforms[3] = {Oscillator::WAVE_POLYBLEP_SQUARE,
                          Oscillator::WAVE_POLYBLEP_TRI,
                          Oscillator::WAVE_POLYBLEP_SAW};

// Bypass vars
Led led_1, led_2;
bool led1_on = false, led2_on = false;

// Audio callback function
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle LEDs based on footswitches
  led1_on ^= hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge();
  led2_on ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // Process each oscillator
  for (int i = 0; i < NUM_OSCILLATORS; i++) {
    // Set up oscillators' frequency and amplitude based on knobs
    float freq = hw.GetKnobValue(freq_knobs[i]) * 1000.0f + 20.0f;
    float amp = hw.GetKnobValue(amp_knobs[i]);
    oscillators[i].SetFreq(freq);
    oscillators[i].SetAmp(amp);

    // Set waveform using the position of the toggle switches
    Hothouse::ToggleswitchPosition switch_pos =
        hw.GetToggleswitchPosition(toggle_switches[i]);
    oscillators[i].SetWaveform(waveforms[switch_pos]);
  }

  // Generate and mix audio output from the oscillators
  for (size_t i = 0; i < size; ++i) {
    float osc_output = 0.0f;
    for (int j = 0; j < NUM_OSCILLATORS; j++) {
      osc_output += oscillators[j].Process();
    }
    osc_output /= NUM_OSCILLATORS;      // Normalize the mixed output
    out[0][i] = in[0][i] + osc_output;  // Left channel + left input
    out[1][i] = in[1][i] + osc_output;  // Right channel + right input
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  led_1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_2.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  // Initialize oscillators
  for (int i = 0; i < NUM_OSCILLATORS; i++) {
    oscillators[i].Init(hw.seed.AudioSampleRate());
  }

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  // Main loop
  while (true) {
    hw.DelayMs(10);

    // Toggle LEDs
    led_1.Set(led1_on ? 1.0f : 0.0f);
    led_1.Update();
    led_2.Set(led2_on ? 1.0f : 0.0f);
    led_2.Update();

    hw.CheckResetToBootloader();
  }

  return 0;
}
