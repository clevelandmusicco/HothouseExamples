// TapeSimulator for Hothouse DIY DSP Platform
// Copyright (C) 2026 David Hilowitz <dhilowitz@gmail.com>
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
#include "StkPitchShift.h"
#include "fast_math.h"

using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::SaiHandle;
using namespace daisysp;

Hothouse hw;

// Bypass vars
Led led_bypass;
bool bypass = true;

// Tape effect parameters
float wowAmount = 0.0f;
float flutterAmount = 0.0f;
float saturationAmount = 0.0f;
float noiseAmount = 0.0f;
float tapeMix = 0.5f;
float filterKnob = 0.5f;

float lowpassFilterFreq = 1000.0f;
float highpassFilterFreq = 1000.0f;

// Wow and Flutter LFOs
Oscillator wowLFO;
Oscillator wowModulatorLFO;
Oscillator flutterLFO;
float lastWowFreq = 1.2f;
float smoothedPitchShift = 1.0f;

// Pitch shifters for each channel
StkPitchShift pitchShifterL(1024);
StkPitchShift pitchShifterR(1024);

// Noise generator
WhiteNoise noise;

// Filters (state variable filters for LP/HP)
Svf lowpassFilterL;
Svf lowpassFilterR;
Svf highpassFilterL;
Svf highpassFilterR;

// Dry buffer for mixing
float dryBufferL[48];
float dryBufferR[48];

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hw.ProcessAllControls();

  // Toggle bypass when FOOTSWITCH_2 is pressed
  bypass ^= hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge();

  // Read controls
  wowAmount = hw.knobs[Hothouse::KNOB_1].Process();        // Knob 1: Wow
  tapeMix = hw.knobs[Hothouse::KNOB_2].Process();          // Knob 2: Dry/Wet
  filterKnob = hw.knobs[Hothouse::KNOB_3].Process();       // Knob 3: Lowpass/Highpass
  flutterAmount = hw.knobs[Hothouse::KNOB_4].Process();    // Knob 4: Flutter
  saturationAmount = hw.knobs[Hothouse::KNOB_5].Process(); // Knob 5: Saturation
  noiseAmount = hw.knobs[Hothouse::KNOB_6].Process();      // Knob 6: Noise
  
  // Calculate filter mode and frequency
  const float deadZone = 0.02f;  // Small dead zone around center
  float filterBlend = 0.0f;  // 0.0 = lowpass, 1.0 = highpass
  
  lowpassFilterFreq = 22000.0f;  // Max frequency when not in lowpass mode
  if (filterKnob < (0.5f - deadZone)) {
    // Lowpass mode: 0.0-0.48 maps to 300Hz-22kHz
    float normalized = filterKnob / (0.5f - deadZone);  // 0.0-1.0
    // Use power curve to emphasize low frequency range (300Hz-1kHz)
    float curved = normalized * normalized;  // Square for more low-freq emphasis
    lowpassFilterFreq = 300.0f * FastMath::pow(22000.0f / 300.0f, curved); 
  }
  
  highpassFilterFreq = 20.0f;  // Min frequency when not in highpass mode
  if (filterKnob > (0.5f + deadZone)) {
    // Highpass mode: 0.52-1.0 maps to 20Hz-2000Hz
    float normalized = (filterKnob - 0.5f - deadZone) / (0.5f - deadZone);  // 0.0-1.0
    highpassFilterFreq = 20.0f * FastMath::pow(2000.0f / 20.0f, normalized);
    
  } 
  
  // Update filter parameters (always, to avoid clicks)
  lowpassFilterL.SetFreq(lowpassFilterFreq);
  lowpassFilterR.SetFreq(lowpassFilterFreq);
  highpassFilterL.SetFreq(highpassFilterFreq);
  highpassFilterR.SetFreq(highpassFilterFreq);
  

  for (size_t i = 0; i < size; ++i) {
    if (bypass) {
      // Copy left input to both outputs (mono-to-dual-mono)
      out[0][i] = out[1][i] = in[0][i];
    } else {
      // Store dry signal for mix
      dryBufferL[i] = in[0][i];
      dryBufferR[i] = in[1][i];
      
      float wetL = in[0][i];
      float wetR = in[1][i];
      
      // Process wow and flutter (pitch modulation)
      
      // Calculate combined pitch modulation with nested modulation for wow
      float wowModulator = wowModulatorLFO.Process();
      float wowFreqVariation = 0.7f + (wowModulator * 0.4f);
      float newWowFreq = 1.2f * wowFreqVariation;
      
      // Only update frequency if it changed significantly (avoid phase resets)
      if (fabsf(newWowFreq - lastWowFreq) > 0.01f) {
        wowLFO.SetFreq(newWowFreq);
        lastWowFreq = newWowFreq;
      }
      
      float wowMod = wowLFO.Process() * wowAmount * 0.04f;
      float flutterMod = flutterLFO.Process() * flutterAmount * 0.02f;
      float targetPitchShift = 1.0f + wowMod + flutterMod;
      
      // Apply exponential smoothing (one-pole lowpass) to reduce zipper noise
      const float smoothingCoeff = 0.95f;  // Higher = more smoothing
      smoothedPitchShift = smoothedPitchShift * smoothingCoeff + targetPitchShift * (1.0f - smoothingCoeff);
      
      pitchShifterL.SetShift(smoothedPitchShift);
      pitchShifterR.SetShift(smoothedPitchShift);
      
      wetL = pitchShifterL.Process(wetL);
      wetR = pitchShifterR.Process(wetR);
      
      
      // Apply tape saturation using fast tanh
      if (saturationAmount > 0.001f) {
        float drive = 1.0f + (saturationAmount * 19.0f);
        wetL = FastMath::tanh(wetL * drive) / drive;
        wetR = FastMath::tanh(wetR * drive) / drive;
      }
      
      // Add tape noise
      if (noiseAmount > 0.001f) {
        float noiseVal = noise.Process() * noiseAmount * 0.004f;
        wetL += noiseVal;
        wetR += noiseVal;
      }
      
      // Apply filtering (always process and use output to avoid clicks)
      lowpassFilterL.Process(wetL);
      lowpassFilterR.Process(wetR);
      wetL = lowpassFilterL.Low();
      wetR = lowpassFilterR.Low();
      highpassFilterL.Process(wetL);
      highpassFilterR.Process(wetR);
      wetL = highpassFilterL.High();
      wetR = highpassFilterR.High();

      // Apply dry/wet mix
      out[0][i] = wetL * tapeMix + dryBufferL[i] * (1.0f - tapeMix);
      out[1][i] = wetR * tapeMix + dryBufferR[i] * (1.0f - tapeMix);
    }
  }
}

int main() {
  hw.Init();
  hw.SetAudioBlockSize(48);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  float sampleRate = hw.AudioSampleRate();

  led_bypass.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  // Initialize pitch shifters
  pitchShifterL.SetEffectMix(1.0f);
  pitchShifterR.SetEffectMix(1.0f);
  
  // Initialize wow/flutter LFOs
  wowLFO.Init(sampleRate);
  wowLFO.SetWaveform(Oscillator::WAVE_SIN);
  wowLFO.SetFreq(1.2f);
  wowLFO.SetAmp(1.0f);
  
  wowModulatorLFO.Init(sampleRate);
  wowModulatorLFO.SetWaveform(Oscillator::WAVE_SIN);
  wowModulatorLFO.SetFreq(0.3f);
  wowModulatorLFO.SetAmp(1.0f);
  
  flutterLFO.Init(sampleRate);
  flutterLFO.SetWaveform(Oscillator::WAVE_SIN);
  flutterLFO.SetFreq(6.0f);
  flutterLFO.SetAmp(1.0f);
  
  // Initialize noise generator
  noise.Init();
  
  // Initialize filters
  lowpassFilterL.Init(sampleRate);
  lowpassFilterR.Init(sampleRate);
  lowpassFilterL.SetFreq(1000.0f);
  lowpassFilterR.SetFreq(1000.0f);
  lowpassFilterL.SetRes(0.1f);
  lowpassFilterR.SetRes(0.1f);
  highpassFilterL.Init(sampleRate);
  highpassFilterR.Init(sampleRate);
  highpassFilterL.SetFreq(1000.0f);
  highpassFilterR.SetFreq(1000.0f);
  highpassFilterL.SetRes(0.1f);
  highpassFilterR.SetRes(0.1f);

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (true) {
    hw.DelayMs(10);

    // Toggle effect bypass LED when footswitch is pressed
    led_bypass.Set(bypass ? 0.0f : 1.0f);
    led_bypass.Update();

    // Call System::ResetToBootloader() if FOOTSWITCH_1 is pressed for 2 seconds
    hw.CheckResetToBootloader();
  }
  return 0;
}