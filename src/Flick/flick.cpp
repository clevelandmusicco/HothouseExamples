// Flick for Hothouse DIY DSP Platform
// Copyright (C) 2024 Boyd Timothy <btimothy@gmail.com>
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
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daisy.h"
#include "daisysp.h"
#include "flick_oscillator.h"
#include "hothouse.h"
#include "Dattorro.hpp"
#include <math.h>

using clevelandmusicco::FlickOscillator;
using clevelandmusicco::Hothouse;
using daisy::AudioHandle;
using daisy::Led;
using daisy::Parameter;
using daisy::PersistentStorage;
using daisy::SaiHandle;
using daisy::System;
using daisysp::DelayLine;
using daisysp::fonepole;

/// Increment this when changing the settings struct so the software will know
/// to reset to defaults if this ever changes.
#define SETTINGS_VERSION 1

Hothouse hw;

#define MAX_DELAY static_cast<size_t>(48000 * 2.0f) // 4 second max delay

enum PedalMode {
  PEDAL_MODE_NORMAL,
  PEDAL_MODE_EDIT_REVERB,     // Edit mode activated by double-press of the left foot switch
  PEDAL_MODE_EDIT_MONO_STEREO // Edit mode activated by long-press of the right foot switch
};

enum MonoStereoMode {                       // Controlled by Toggle Switch 3
  MS_MODE_MIMO, // Mono In, Mono Out        // TOGGLESWITCH_DOWN
  MS_MODE_MISO, // Mono In, Stereo Out      // TOGGLESWITCH_MIDDLE
  MS_MODE_SISO  // Stereo In, Stereo Out    // TOGGLESWITCH_UP
};

// Persistent Settings
struct Settings {
  int version; // Version of the settings struct
  float decay;
  float diffusion;
  float inputCutoffFreq;
  float tankCutoffFreq;
  float tankModSpeed;
  float tankModDepth;
  float tankModShape;
  float preDelay;
  int monoStereoMode;

	//Overloading the != operator
	//This is necessary as this operator is used in the PersistentStorage source code
	bool operator!=(const Settings& a) const {
    return !(
      a.version == version &&
      a.decay == decay &&
      a.diffusion == diffusion &&
      a.inputCutoffFreq == inputCutoffFreq &&
      a.tankCutoffFreq == tankCutoffFreq &&
      a.tankModSpeed == tankModSpeed &&
      a.tankModDepth == tankModDepth &&
      a.tankModShape == tankModShape &&
      a.preDelay == preDelay &&
      a.monoStereoMode == monoStereoMode
    );
  }
};

//Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<Settings> SavedSettings(hw.seed.qspi);

FlickOscillator osc;
float dc_os = 0;

DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delMemL;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delMemR;

Dattorro verb(48000, 16, 4.0);
PedalMode pedal_mode = PEDAL_MODE_NORMAL;
MonoStereoMode mono_stereo_mode = MS_MODE_MIMO;

Parameter p_verb_amt;
Parameter p_trem_speed, p_trem_depth;
Parameter p_delay_time, p_delay_feedback, p_delay_amt;

Parameter p_knob_1, p_knob_2, p_knob_3, p_knob_4, p_knob_5, p_knob_6;

struct Delay {
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

enum ReverbKnobMode {
  REVERB_KNOB_ALL_DRY,
  REVERB_KNOB_DRY_WET_MIX,
  REVERB_KNOB_ALL_WET,
};

enum TremDelMakeUpGain {
  TV_MAKEUP_GAIN_NONE,
  TV_MAKEUP_GAIN_NORMAL,
  TV_MAKEUP_GAIN_HEAVY,
};

constexpr ReverbKnobMode kReverbKnobMap[] = {
  REVERB_KNOB_ALL_WET,                        // UP
  REVERB_KNOB_DRY_WET_MIX,                    // MIDDLE
  REVERB_KNOB_ALL_DRY,                        // DOWN
};

constexpr TremDelMakeUpGain kMakeupGainMap[] = {
  TV_MAKEUP_GAIN_HEAVY,                       // UP
  TV_MAKEUP_GAIN_NORMAL,                      // MIDDLE
  TV_MAKEUP_GAIN_NONE,                        // DOWN
};

constexpr int kWaveformMap[] = {
    FlickOscillator::WAVE_SQUARE_ROUNDED,     // UP
    FlickOscillator::WAVE_TRI,                // MIDDLE
    FlickOscillator::WAVE_SIN,                // DOWN
};

Delay delayL;
Delay delayR;
int delay_drywet;

float reverb_tone;
float reverb_feedback;
float reverb_sploodge;

// Bypass vars
Led led_left, led_right;
bool bypass_verb = true;
bool bypass_trem = true;
bool bypass_delay = true;

// Reverb vars
bool plateDiffusionEnabled = true;
float platePreDelay = 0.;

float plateDelay = 0.0;

float plateDry = 1.0;
float plateWet = 0.5;

float plateDecay = 0.8;
float plateTimeScale = 1.007500;

float plateTankDiffusion = 0.85;

  /**
   * Good Defaults
   * Lo Pitch: .287 (2.87) = 100Hz: 440 * (2^(2.87-5))
   * InputFilterHighCutoffPitch: 0.77 (7.77) is approx 3000Hz
   * TankFilterHighCutFrequency: 0.8 (8.0) is 3520Hz
   * 0.9507 is approx 10kHz
   * 
   * mod speed: 0.5
   * mod depth: 0.5
   * mod shape: 0.75
   */

// The damping values appear to be want to be between 0 and 10
float plateInputDampLow = 2.87; // approx 100Hz
float plateInputDampHigh = 7.25;

float plateTankDampLow = 2.87; // approx 100Hz
float plateTankDampHigh = 7.25;

float plateTankModSpeed = 0.1;
float plateTankModDepth = 0.1;
float plateTankModShape = 0.25;

const float minus18dBGain = 0.12589254;
const float minus20dBGain = 0.1;

float leftInput = 0.;
float rightInput = 0.;
float leftOutput = 0.;
float rightOutput = 0.;

float inputAmplification = 1.0; // This isn't really used yet

bool trigger_settings_save = false;

/// @brief Used at startup to control a factory reset.
///
/// This gets set to true in `main()` if footswitch 2 is depressed at boot.
/// The LED lights will start flashing alternatively. To exit this mode without
/// making any changes, press either footswitch.
///
/// To reset, rotate knob_1 to 100%, to 0%, to 100%, and back to 0%. This will
/// restore all defaults and then go into normal pedal mode.
bool is_factory_reset_mode = false;

/// @brief Tracks the stage of knob_1 rotation in factory reset mode.
///
/// 0: User must rotate knob_1 to 100% to advance to the next stage.
/// 1: User must rotate knob_1 to 0% to advance to the next stage.
/// 2: User must rotate knob_1 to 100% to advance to the next stage.
/// 3: User must rotate knob_1 to 0% to complete the factory reset.
int factory_reset_stage = 0;

void load_settings() {

	// Reference to local copy of settings stored in flash
	Settings &LocalSettings = SavedSettings.GetSettings();

  int savedVersion = LocalSettings.version;

  if (savedVersion != SETTINGS_VERSION) {
    // Something has changed. Load defaults!
    SavedSettings.RestoreDefaults();
    load_settings();
    return;
  }

  plateDecay = LocalSettings.decay;
  plateTankDiffusion = LocalSettings.diffusion;
  plateInputDampHigh = LocalSettings.inputCutoffFreq;
  plateTankDampHigh = LocalSettings.tankCutoffFreq;
  plateTankModSpeed = LocalSettings.tankModSpeed;
  plateTankModDepth = LocalSettings.tankModDepth;
  plateTankModShape = LocalSettings.tankModShape;
  platePreDelay = LocalSettings.preDelay;
  mono_stereo_mode = static_cast<MonoStereoMode>(LocalSettings.monoStereoMode);

  verb.setPreDelay(platePreDelay);
  verb.setInputFilterHighCutoffPitch(plateInputDampHigh);
  verb.setDecay(plateDecay);
  verb.setTankDiffusion(plateTankDiffusion);
  verb.setTankFilterHighCutFrequency(plateTankDampHigh);
  verb.setTankModSpeed(plateTankModSpeed * 8);
  verb.setTankModDepth(plateTankModDepth * 15);
  verb.setTankModShape(plateTankModShape);
}

void save_settings() {
	//Reference to local copy of settings stored in flash
	Settings &LocalSettings = SavedSettings.GetSettings();

  LocalSettings.version = SETTINGS_VERSION;
  LocalSettings.decay = plateDecay;
  LocalSettings.diffusion = plateTankDiffusion;
  LocalSettings.inputCutoffFreq = plateInputDampHigh;
  LocalSettings.tankCutoffFreq = plateTankDampHigh;
  LocalSettings.tankModSpeed = plateTankModSpeed;
  LocalSettings.tankModDepth = plateTankModDepth;
  LocalSettings.tankModShape = plateTankModShape;
  LocalSettings.preDelay = platePreDelay;

	trigger_settings_save = true;
}

void save_mono_stereo_settings() {
  Settings &LocalSettings = SavedSettings.GetSettings();

  LocalSettings.monoStereoMode = mono_stereo_mode;

  trigger_settings_save = true;
}

/// @brief Restore the reverb settings from the saved settings.
void restore_reverb_settings() {
	Settings &LocalSettings = SavedSettings.GetSettings();

  plateDecay = LocalSettings.decay;
  plateTankDiffusion = LocalSettings.diffusion;
  plateInputDampHigh = LocalSettings.inputCutoffFreq;
  plateTankDampHigh = LocalSettings.tankCutoffFreq;
  plateTankModSpeed = LocalSettings.tankModSpeed;
  plateTankModDepth = LocalSettings.tankModDepth;
  plateTankModShape = LocalSettings.tankModShape;
  platePreDelay = LocalSettings.preDelay;

  verb.setDecay(plateDecay);
  verb.setTankDiffusion(plateTankDiffusion);
  verb.setInputFilterHighCutoffPitch(plateInputDampHigh);
  verb.setTankFilterHighCutFrequency(plateTankDampHigh);

  verb.setTankModSpeed(plateTankModSpeed * 8);
  verb.setTankModDepth(plateTankModDepth * 15);
  verb.setTankModShape(plateTankModShape);
  verb.setPreDelay(platePreDelay);    
}

/// @brief Restore the mono-stereo settings from the saved settings.
void restore_mono_stereo_settings() {
  Settings &LocalSettings = SavedSettings.GetSettings();

  mono_stereo_mode = static_cast<MonoStereoMode>(LocalSettings.monoStereoMode);
}

void handle_normal_press(Hothouse::Switches footswitch) {
  if (pedal_mode == PEDAL_MODE_EDIT_REVERB) {
    // Only save the settings if the RIGHT footswitch is pressed in edit mode.
    // The LEFT footswitch is used to exit edit mode without saving.
    if (footswitch == Hothouse::FOOTSWITCH_2) {
      // Save the settings
      save_settings();
    } else {
      restore_reverb_settings();
    }
    pedal_mode = PEDAL_MODE_NORMAL;
  } else if (pedal_mode == PEDAL_MODE_EDIT_MONO_STEREO) {
    // Only save the settings if the RIGHT footswitch is pressed in mono-stereo
    // edit mode. The LEFT footswitch is used to exit mono-stereo edit mode
    // without saving.
    if (footswitch == Hothouse::FOOTSWITCH_2) {
      // Save the mono-stereo settings
      save_mono_stereo_settings();
    } else {
      restore_mono_stereo_settings();
    }
    pedal_mode = PEDAL_MODE_NORMAL;
  } else {
    if (footswitch == Hothouse::FOOTSWITCH_1) {
      bypass_verb = !bypass_verb;

      if (bypass_verb) {
        // Clear the reverb tails when the reverb is bypassed so if you
        // turn it back on, it starts fresh and doesn't sound weird.
        verb.clear();
      }
    } else {
      bypass_delay = !bypass_delay;
    }
  }
}

void handle_double_press(Hothouse::Switches footswitch) {
  // Ignore double presses in edit modes
  if (pedal_mode == PEDAL_MODE_EDIT_REVERB || pedal_mode == PEDAL_MODE_EDIT_MONO_STEREO) {
    return;
  }

  // When double press is detected, a normal press was already detected and
  // processed, so reverse that right off the bat.
  handle_normal_press(footswitch);

  if (footswitch == Hothouse::FOOTSWITCH_1) {
    // Go into reverb edit mode
    bypass_verb = false; // Make sure that reverb is ON
    pedal_mode = PEDAL_MODE_EDIT_REVERB;
  } else if (footswitch == Hothouse::FOOTSWITCH_2) {
    // Toggle the trem bypass
    bypass_trem = !bypass_trem;
  }
}

void handle_long_press(Hothouse::Switches footswitch) {
  if (footswitch == Hothouse::FOOTSWITCH_2) {
    // If the right footswitch is long-pressed, enter mono-stereo config.
    pedal_mode = PEDAL_MODE_EDIT_MONO_STEREO;
  }
}

inline float hardLimit100_(const float &x) {
    return (x > 1.) ? 1. : ((x < -1.) ? -1. : x);
}

void quick_led_flash() {
  led_left.Set(1.0f);
  led_right.Set(1.0f);
  led_left.Update();
  led_right.Update();
  hw.DelayMs(500);
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  static float trem_val;
  hw.ProcessAllControls();

  if (pedal_mode == PEDAL_MODE_EDIT_REVERB) {
    // Edit mode

    // Blink the left & right LEDs
    {
      static uint32_t edit_count = 0;
      static bool led_state = true;
      if (++edit_count >= hw.AudioCallbackRate() / 2) {
        edit_count = 0;
        led_state = !led_state;
        led_left.Set(led_state ? 1.0f : 0.0f);
        led_right.Set(led_state ? 1.0f : 0.0f);
      }
    }
  } else if (pedal_mode == PEDAL_MODE_EDIT_MONO_STEREO) {
    // Mono-Stereo edit mode
    // Blink the left & right LEDs alternately to indicate mono-stereo edit mode
    static uint32_t mono_stereo_edit_count = 0;
    static bool led_state = true;
    if (++mono_stereo_edit_count >= hw.AudioCallbackRate() / 2) {
      mono_stereo_edit_count = 0;
      led_state = !led_state;
      led_left.Set(led_state ? 1.0f : 0.0f);
      led_right.Set(led_state ? 0.0f : 1.0f);
    }
  } else {
    // Normal mode
    led_left.Set(bypass_verb ? 0.0f : 1.0f);

    // Reduce number of LED Updates for pulsing trem LED
    {
      static int count = 0;
      // set led 100 times/sec
      if (++count == hw.AudioCallbackRate() / 100) {
        count = 0;
        // If just delay is on, show full-strength LED
        // If just trem is on, show 40% pulsing LED
        // If both are on, show 100% pulsing LED
        led_right.Set(bypass_trem ? bypass_delay ? 0.0f : 1.0 : bypass_delay ? trem_val * 0.4 : trem_val);
      }
    }
  }
  led_left.Update();
  led_right.Update();

  plateWet = p_verb_amt.Process();

  TremDelMakeUpGain makeup_gain = kMakeupGainMap[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3)];

  if (pedal_mode == PEDAL_MODE_NORMAL) {
    osc.SetFreq(p_trem_speed.Process());
    static float depth = 0;
    depth = daisysp::fclamp(p_trem_depth.Process(), 0.f, 1.f);
    depth *= 0.5f;
    osc.SetAmp(depth);
    dc_os = 1.f - depth;

    osc.SetWaveform(kWaveformMap[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)]);

    //
    // Delay
    //
    delayL.delayTarget = delayR.delayTarget =  p_delay_time.Process();
    delayL.feedback = delayR.feedback = p_delay_feedback.Process();
    delay_drywet = (int)p_delay_amt.Process();

    // Reverb dry/wet mode
    switch (kReverbKnobMap[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)]) {
      case REVERB_KNOB_ALL_DRY:
        plateDry = 1.0;
        break;
      case REVERB_KNOB_DRY_WET_MIX:
        plateDry = 1.0 - plateWet;
        break;
      case REVERB_KNOB_ALL_WET:
        plateDry = 0.0f;
        break;
    }
  } else if (pedal_mode == PEDAL_MODE_EDIT_REVERB) {
    // Edit mode
    plateDry = 1.0; // Always use dry 100% in edit mode
    platePreDelay = p_knob_2.Process() * 0.25;
    plateDecay = p_knob_3.Process();        
    plateTankDiffusion = p_knob_4.Process();
    plateInputDampHigh = p_knob_5.Process() * 10.0; // Dattorro takes values for this between 0 and 10
    plateTankDampHigh = p_knob_6.Process() * 10.0; // Dattorro takes values for this between 0 and 10

    //
    // Read in all of the toggle switch values
    //

    // Switch 1 - Tank Mod Speed
    static const float tank_mod_speed_values[] = {0.5f, 0.25f, 0.1f};
    plateTankModSpeed = tank_mod_speed_values[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)];

    // Switch 2 - Tank Mod Depth
    static const float tank_mod_depth_values[] = {0.5f, 0.25f, 0.1f};
    plateTankModDepth = tank_mod_depth_values[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2)];

    // Switch 3 - Tank Mod Shape
    static const float tank_mod_shape_values[] = {0.5f, 0.25f, 0.1f};
    plateTankModShape = tank_mod_shape_values[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3)];

    verb.setDecay(plateDecay);
    verb.setTankDiffusion(plateTankDiffusion);
    verb.setInputFilterHighCutoffPitch(plateInputDampHigh);
    verb.setTankFilterHighCutFrequency(plateTankDampHigh);

    verb.setTankModSpeed(plateTankModSpeed * 8);
    verb.setTankModDepth(plateTankModDepth * 15);
    verb.setTankModShape(plateTankModShape);
    verb.setPreDelay(platePreDelay);    
  } else if (pedal_mode == PEDAL_MODE_EDIT_MONO_STEREO) {
    // Mono-Stereo edit mode
    // Read in the mono-stereo mode from toggle switch 3
    mono_stereo_mode = static_cast<MonoStereoMode>(hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3));
    switch (hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3)) {
      case Hothouse::TOGGLESWITCH_MIDDLE:
        mono_stereo_mode = MS_MODE_MISO; // Mono In, Stereo Out
        break;
      case Hothouse::TOGGLESWITCH_UP:
        mono_stereo_mode = MS_MODE_SISO; // Stereo In, Stereo Out
        break;
      default:
        mono_stereo_mode = MS_MODE_MIMO; // Mono In, Mono Out
    }
  }

  for (size_t i = 0; i < size; ++i) {
    float dry_L = in[0][i];
    float dry_R = in[1][i];
    float s_L, s_R;
    s_L = dry_L;
    if (mono_stereo_mode == MS_MODE_MIMO || mono_stereo_mode == MS_MODE_MISO) {
      // Use the mono signel (L) for both channels in MIMO and MISO modes
      s_R = dry_L;
    } else {
      // Use both L & R inputs in SISO mode
      s_R = dry_R;
    }

    if (!bypass_delay) {
      float mixL = 0;
      float mixR = 0;
      float fdrywet = delay_drywet / 100.0f;

      // update delayline with feedback
      float sigL = delayL.Process(s_L);
      float sigR = delayR.Process(s_R);
      mixL += sigL;
      mixR += sigR;

      float delay_make_up_gain = makeup_gain == TV_MAKEUP_GAIN_NONE ? 1.0f : makeup_gain == TV_MAKEUP_GAIN_NORMAL ? 1.66f : 2.0f;

      // apply drywet and attenuate
      s_L = fdrywet * mixL * 0.333f + (1.0f - fdrywet) * s_L * delay_make_up_gain;
      s_R = fdrywet * mixR * 0.333f + (1.0f - fdrywet) * s_R * delay_make_up_gain;
    }

    if (!bypass_trem) {
      // trem_val gets used above for pulsing LED
      trem_val = dc_os + osc.Process();
      float trem_make_up_gain = makeup_gain == TV_MAKEUP_GAIN_NONE ? 1.0f : makeup_gain == TV_MAKEUP_GAIN_NORMAL ? 1.2f : 1.6f;

      s_L = s_L * trem_val * trem_make_up_gain;
      s_R = s_R * trem_val * trem_make_up_gain;
    }

    // Keep sending input to the reverb even if bypassed so that when it's
    // enabled again it will already have the current input signal already
    // being processed.
    
    // Dattorro seems to want to have values between -10 and 10 so times by 10
    // leftInput = hardLimit100_(s_L) * 10.0f;
    // rightInput = hardLimit100_(s_R) * 10.0f;

    // For now, send in double the amount of input to the reverb which seems
    // to work a little better. Sending in 10x was making the reverb be WAY
    // too loud.
    leftInput = hardLimit100_(s_L) * 2.0f;
    rightInput = hardLimit100_(s_R) * 2.0f;

    verb.process(leftInput * minus18dBGain * minus20dBGain * (1.0f + inputAmplification * 7.0f) * clearPopCancelValue,
                  rightInput * minus18dBGain * minus20dBGain * (1.0f + inputAmplification * 7.0f) * clearPopCancelValue);

    if (!bypass_verb) {
      // leftOutput = ((leftInput * plateDry * 0.1) + (verb.getLeftOutput() * plateWet * clearPopCancelValue));
      // rightOutput = ((rightInput * plateDry * 0.1) + (verb.getRightOutput() * plateWet * clearPopCancelValue));
      leftOutput = ((leftInput * plateDry * 0.5) + (verb.getLeftOutput() * plateWet * clearPopCancelValue));
      rightOutput = ((rightInput * plateDry * 0.5) + (verb.getRightOutput() * plateWet * clearPopCancelValue));

      s_L = leftOutput;
      s_R = rightOutput;
    }

    if (mono_stereo_mode == MS_MODE_MIMO) {
      out[0][i] = (s_L * 0.5) + (s_R * 0.5); // Sum the processed left and right channels
      out[1][i] = 0.0f; // Mute the unused channel
    } else {
      // Send stereo output in MISO and SISO
      out[0][i] = s_L;
      out[1][i] = s_R;
    }
  }
}

int main() {
  hw.Init(true); // Init the CPU at full speed
  hw.SetAudioBlockSize(8);  // Number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  
  // Initialize LEDs
  led_left.Init(hw.seed.GetPin(Hothouse::LED_1), false);
  led_right.Init(hw.seed.GetPin(Hothouse::LED_2), false);

  //
  // Initialize Potentiometers
  //

  // The p_knob_n parameters are used to process the potentiometers when in reverb edit mode.
  p_knob_1.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
  p_knob_2.Init(hw.knobs[Hothouse::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
  p_knob_3.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
  p_knob_4.Init(hw.knobs[Hothouse::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
  p_knob_5.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  p_knob_6.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

  p_verb_amt.Init(hw.knobs[Hothouse::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);

  p_trem_speed.Init(hw.knobs[Hothouse::KNOB_2], 0.2f, 16.0f, Parameter::LINEAR);
  p_trem_depth.Init(hw.knobs[Hothouse::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);

  p_delay_time.Init(hw.knobs[Hothouse::KNOB_4], hw.AudioSampleRate() * 0.05f, MAX_DELAY, Parameter::LOGARITHMIC);
  p_delay_feedback.Init(hw.knobs[Hothouse::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
  p_delay_amt.Init(hw.knobs[Hothouse::KNOB_6], 0.0f, 100.0f, Parameter::LINEAR);

  delMemL.Init();
  delMemR.Init();
  delayL.del = &delMemL;
  delayR.del = &delMemR;

  osc.Init(hw.AudioSampleRate());

  //
  // Dattorro Reverb Initialization
  //
  // Zero out the InterpDelay buffers used by the plate reverb
  for(int i = 0; i < 50; i++) {
      for(int j = 0; j < 144000; j++) {
          sdramData[i][j] = 0.;
      }
  }
  // Set this to 1.0 or plate reverb won't work. This is defined in Dattorro's
  // InterpDelay.cpp file.
  hold = 1.;

  verb.setSampleRate(48000);
  verb.setTimeScale(plateTimeScale);
  verb.enableInputDiffusion(plateDiffusionEnabled);
  verb.setInputFilterLowCutoffPitch(plateInputDampLow);
  verb.setTankFilterLowCutFrequency(plateTankDampLow);

  Settings defaultSettings = {
    SETTINGS_VERSION, // version
    plateDecay,
    plateTankDiffusion,
    plateInputDampHigh,
    plateTankDampHigh,
    plateTankModSpeed,
    plateTankModDepth,
    plateTankModShape,
    platePreDelay
  };
  SavedSettings.Init(defaultSettings);

  load_settings();

  Hothouse::FootswitchCallbacks callbacks = {
    .HandleNormalPress = handle_normal_press,
    .HandleDoublePress = handle_double_press,
    .HandleLongPress = handle_long_press
  };
  hw.RegisterFootswitchCallbacks(&callbacks);

  hw.StartAdc();
  hw.ProcessAllControls();
  static bool enter_mono_stereo_edit_mode = false;
  if (hw.switches[Hothouse::FOOTSWITCH_1].RawState()) {
    // If FOOTSWITCH_1 is pressed go into mono-stereo edit mode
    enter_mono_stereo_edit_mode = true;
  } else if (hw.switches[Hothouse::FOOTSWITCH_2].RawState()) {
    is_factory_reset_mode = true;
  } else {
    hw.StartAudio(AudioCallback);
  }
  
  while (true) {
    if(trigger_settings_save) {
			SavedSettings.Save(); // Writing locally stored settings to the external flash
			trigger_settings_save = false;
		} else if (is_factory_reset_mode) {
      hw.ProcessAllControls();

      static uint32_t last_led_toggle_time = 0;
      static bool led_toggle = false;
      static uint32_t blink_interval = 1000;
      uint32_t now = System::GetNow();
      uint32_t elapsed_time = now - last_led_toggle_time;
      if (elapsed_time >= blink_interval) {
        // Alternate the LED lights in factory reset mode
        last_led_toggle_time = now;
        led_toggle = !led_toggle;
        led_left.Set(led_toggle ? 1.0f : 0.0f);
        led_right.Set(led_toggle ? 0.0f : 1.0f);
        led_left.Update();
        led_right.Update();
      }

      float low_knob_threshold = 0.05;
      float high_knob_threshold = 0.95;
      float blink_faster_amount = 300; // each stage removes this many MS from the factory reset blinking
      float knob_1_value = p_knob_1.Process();
      if (factory_reset_stage == 0 && knob_1_value >= high_knob_threshold) {
        factory_reset_stage++;
        blink_interval -= blink_faster_amount; // make the blinking faster as a UI feedback that the stage has been met
        quick_led_flash();          
      } else if (factory_reset_stage == 1 && knob_1_value <= low_knob_threshold) {
        factory_reset_stage++;
        blink_interval -= blink_faster_amount; // make the blinking faster as a UI feedback that the stage has been met
        quick_led_flash();          
      } else if (factory_reset_stage == 2 && knob_1_value >= high_knob_threshold) {
        factory_reset_stage++;
        blink_interval -= blink_faster_amount; // make the blinking faster as a UI feedback that the stage has been met
        quick_led_flash();          
      } else if (factory_reset_stage == 3 && knob_1_value <= low_knob_threshold) {
        SavedSettings.RestoreDefaults();
        load_settings();
        quick_led_flash();          

        hw.StartAudio(AudioCallback);
        factory_reset_stage = 0;
        bypass_delay = true;
        bypass_trem = true;
        pedal_mode = PEDAL_MODE_NORMAL;
        is_factory_reset_mode = false;
      }
    } else if (enter_mono_stereo_edit_mode && !hw.switches[Hothouse::FOOTSWITCH_1].RawState()) {
      // If we need to enter mono-stereo edit mode, wait for the footswitch to
      // no longer be pressed before entering (because otherwise it'll
      // immediately exit because of the footswitch normal press handler).
    
      enter_mono_stereo_edit_mode = false;
      pedal_mode = PEDAL_MODE_EDIT_MONO_STEREO;
      // Make sure that reverb is ON in edit mode so changes can be heard
      bypass_verb = false;
      hw.StartAudio(AudioCallback);
    }
    hw.DelayMs(10);

    // Call System::ResetToBootloader() if FOOTSWITCH_1 is pressed for 2 seconds
    if (!enter_mono_stereo_edit_mode) {
      hw.CheckResetToBootloader();
    }
  }
  return 0;
}
