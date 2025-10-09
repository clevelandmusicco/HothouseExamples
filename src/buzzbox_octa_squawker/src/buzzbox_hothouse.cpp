// buzzbox_hothouse.cpp
// Buzzbox Octa Squawker - Aggressive Fuzz with Envelope Filter and Octave
// Phase 8 - Complete control redesign with context-dependent knobs
// FIX: Added FS2 volume compensation when fuzz is bypassed

#include "daisy_seed.h"
#include "daisysp.h"
#include "hothouse.h"
#include "buzzbox_hothouse.h"

#include <q/support/literals.hpp>
#include <q/fx/biquad.hpp>
#include "Util/Multirate.h"
#include "Util/OctaveGenerator.h"
namespace q = cycfi::q;
using namespace q::literals;

using namespace daisy;
using namespace daisysp;
using namespace clevelandmusicco;

// Hardware object
Hothouse hw;

// Audio processing objects
Tone tone;
Adsr autowah_adsr;
Svf autowah_svf;
EnvelopeFollower envelopeFollower;

// Octave processing objects
static Decimator2 decimate;
static Interpolator interpolate;
static const auto sample_rate_temp = 48000;
static OctaveGenerator octave(sample_rate_temp / resample_factor);
float octave_buff[6];
float octave_buff_out[6];
int octave_bin_counter = 0;

// Control variables
float knobValues[6] = {0.0f};
float prevKnobValues[6] = {0.0f};
bool knob_touched[6] = {true, true, true, true, true, true};  // Start as touched
int toggleValues[3] = {1};
int prev_toggleValues[3] = {-1, -1, -1};

// Effect parameters - context dependent
float drive_amount = 1.0f;
float tone_freq = 800.0f;
float gate_threshold = 0.0f;
float autowah_speed = 0.5f;
float autowah_threshold = 0.5f;
float autowah_range = 0.5f;
float octave_up_level = 0.5f;
float octave_down_level = 0.5f;
float octave_mix = 0.5f;

// Effect state
bool fuzz_enabled = false;
bool autowah_enabled = false;
bool octave_enabled = false;
int autowah_placement = 0; // 0=before fuzz, 1=after fuzz, 2=after everything

// LEDs
Led led1, led2;

// First start flag for initialization
bool first_start = true;

void updateSwitch1() {
    // Switch 1: Autowah Placement
    // UP (0) = before fuzz, MIDDLE (1) = after fuzz, DOWN (2) = after everything
    autowah_placement = toggleValues[0];
}

void updateSwitch2() {
    // Switch 2: Effect Selection for FS2
    // UP (0) = Both autowah + octave
    // MIDDLE (1) = Autowah only
    // DOWN (2) = Octave only
    
    // If FS2 is currently engaged, update effect states to match new Switch 2 position
    if (autowah_enabled || octave_enabled) {
        switch(toggleValues[1]) {
            case 0: // UP: Both
                autowah_enabled = true;
                octave_enabled = true;
                break;
            case 1: // MIDDLE: Autowah only
                autowah_enabled = true;
                octave_enabled = false;
                break;
            case 2: // DOWN: Octave only
                octave_enabled = true;
                autowah_enabled = false;
                break;
        }
    }
}

void updateSwitch3() {
    // Switch 3: Knob 4-6 Function Mode
    // UP (0) = Fuzz controls
    // MIDDLE (1) = Autowah controls
    // DOWN (2) = Octave controls
    
    // Update parameters based on current knob positions
    // Only update if knob has been touched
    switch(toggleValues[2]) {
        case 0: // Fuzz controls
            // Knob 4: Drive (combined gain + intensity)
            if (knob_touched[3]) {
                drive_amount = knobValues[3];
            }
            // Knob 5: Tone
            if (knob_touched[4]) {
                tone_freq = ParameterRanges::TONE_FREQ_MIN + 
                           (knobValues[4] * (ParameterRanges::TONE_FREQ_MAX - ParameterRanges::TONE_FREQ_MIN));
            }
            // Knob 6: Gate threshold
            if (knob_touched[5]) {
                gate_threshold = knobValues[5];
            }
            break;
            
        case 1: // Autowah controls
            // Knob 4: Attack/Release Speed (linked)
            if (knob_touched[3]) {
                autowah_speed = knobValues[3];
                float attack_time = 0.01f + (autowah_speed * 0.19f);   // 10ms to 200ms
                float release_time = 0.02f + (autowah_speed * 0.38f);  // 20ms to 400ms
                autowah_adsr.SetAttackTime(attack_time);
                autowah_adsr.SetReleaseTime(release_time);
            }
            // Knob 5: Threshold (trigger sensitivity)
            if (knob_touched[4]) {
                autowah_threshold = knobValues[4];
            }
            // Knob 6: Filter Range (shifts base 300-2000Hz range)
            if (knob_touched[5]) {
                autowah_range = knobValues[5];
            }
            break;
            
        case 2: // Octave controls
            // Knob 4: Octave Up Level
            if (knob_touched[3]) {
                octave_up_level = knobValues[3];
            }
            // Knob 5: Octave Down Level
            if (knob_touched[4]) {
                octave_down_level = knobValues[4];
            }
            // Knob 6: Overall Octave Mix
            if (knob_touched[5]) {
                octave_mix = knobValues[5];
            }
            break;
    }
}

void UpdateLEDs() {
    led1.Set(fuzz_enabled ? 1.0f : 0.0f);
    led2.Set((autowah_enabled || octave_enabled) ? 1.0f : 0.0f);
    
    led1.Update();
    led2.Update();
}

void ProcessControls() {
    hw.ProcessAllControls();
    
    // Read all knobs
    knobValues[0] = hw.GetKnobValue(Hothouse::KNOB_1);
    knobValues[1] = hw.GetKnobValue(Hothouse::KNOB_2);
    knobValues[2] = hw.GetKnobValue(Hothouse::KNOB_3);
    knobValues[3] = hw.GetKnobValue(Hothouse::KNOB_4);
    knobValues[4] = hw.GetKnobValue(Hothouse::KNOB_5);
    knobValues[5] = hw.GetKnobValue(Hothouse::KNOB_6);
    
    // Detect knob movement for touch-to-activate behavior
    // Only check knobs 4-6 (context-dependent knobs)
    for(int i = 3; i < 6; i++) {
        if (std::abs(knobValues[i] - prevKnobValues[i]) > 0.01f) {
            knob_touched[i] = true;  // Knob has moved
            prevKnobValues[i] = knobValues[i];  // Only update after detecting movement
        }
    }
    
    // Read all toggle switches
    toggleValues[0] = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
    toggleValues[1] = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
    toggleValues[2] = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
    
    // Update switch functions if they changed
    if (toggleValues[0] != prev_toggleValues[0] || first_start) {
        prev_toggleValues[0] = toggleValues[0];
        updateSwitch1();
    }
    
    if (toggleValues[1] != prev_toggleValues[1] || first_start) {
        prev_toggleValues[1] = toggleValues[1];
        updateSwitch2();
    }
    
    if (toggleValues[2] != prev_toggleValues[2] || first_start) {
        prev_toggleValues[2] = toggleValues[2];
        
        // Reset touch flags ONLY when toggle actually changes (not on first start)
        if (!first_start) {
            knob_touched[3] = false;
            knob_touched[4] = false;
            knob_touched[5] = false;
            // Capture current knob positions so we can detect movement from here
            prevKnobValues[3] = knobValues[3];
            prevKnobValues[4] = knobValues[4];
            prevKnobValues[5] = knobValues[5];
        }
        
        updateSwitch3();
    }
    
    // Update context-dependent parameters continuously
    updateSwitch3();
    
    first_start = false;
    
    // Footswitch 1: Fuzz On/Off
    if (hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
        fuzz_enabled = !fuzz_enabled;
    }
    
    // Footswitch 2: Autowah/Octave based on Switch 2
    if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
        switch(toggleValues[1]) {
            case 0: { // UP: Both
                bool new_state = !(autowah_enabled || octave_enabled);
                autowah_enabled = new_state;
                octave_enabled = new_state;
                break;
            }
            case 1: // MIDDLE: Autowah only
                autowah_enabled = !autowah_enabled;
                octave_enabled = false;
                break;
            case 2: // DOWN: Octave only
                octave_enabled = !octave_enabled;
                autowah_enabled = false;
                break;
        }
    }
    
    UpdateLEDs();
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    ProcessControls();
    
    for (size_t i = 0; i < size; i++) {
        float input = in[0][i];
        float signal = input;
        
        // STAGE 1: Input Gain (Knob 1) - affects everything
        const float input_gain = 0.5f + (knobValues[0] * 1.5f); // 0.5x to 2.0x
        signal *= input_gain;
        
        // STAGE 2: Autowah BEFORE fuzz (if placement is UP and enabled)
        if (autowah_enabled && autowah_placement == 0) {
            // Envelope detection
            float envelope = envelopeFollower.Process(signal);
            
            // Gate ADSR based on envelope level and threshold
            float gate_level = 0.05f + (autowah_threshold * 0.15f); // 0.05 to 0.20
            bool gate = (envelope > gate_level);
            float adsr_out = autowah_adsr.Process(gate);
            
            // Map ADSR output to filter frequency with range control
            // Base range: 300-2000Hz
            // Range knob shifts this: CCW = 100-1100Hz, CW = 300-3000Hz
            float range_min = 100.0f + (autowah_range * 200.0f);  // 100-300Hz
            float range_max = 1100.0f + (autowah_range * 1900.0f); // 1100-3000Hz
            float filter_freq = range_min + (adsr_out * (range_max - range_min));
            
            autowah_svf.SetFreq(filter_freq);
            autowah_svf.SetRes(0.7f);
            
            // Process through SVF and use bandpass output with gain compensation
            autowah_svf.Process(signal);
            signal = autowah_svf.Band() * 2.0f;  // Autowah makeup gain
        }
        
        // STAGE 3: Octave processing (if enabled)
        if (octave_enabled) {
            // Buffer input for octave processing
            octave_buff[octave_bin_counter] = signal;
            
            // Process octave every 6 samples
            if (octave_bin_counter == 5) {
                std::span<const float, resample_factor> in_chunk(&(octave_buff[0]), resample_factor);
                const auto sample = decimate(in_chunk);
                
                octave.update(sample);
                
                // Mix up and down octaves with individual level controls
                float octave_signal = octave.up1() * octave_up_level * 1.5f + 
                                     octave.down1() * octave_down_level * 1.5f;
                
                auto out_chunk = interpolate(octave_signal);
                for (size_t j = 0; j < out_chunk.size(); ++j) {
                    // Mix octave with dry based on octave_mix
                    octave_buff_out[j] = octave_buff[j] * (1.0f - octave_mix) + 
                                        out_chunk[j] * octave_mix;
                }
            }
            
            // Use octave-processed signal if available
            if (octave_bin_counter < 6) {
                signal = octave_buff_out[octave_bin_counter];
            }
            
            // Update bin counter
            octave_bin_counter++;
            if (octave_bin_counter >= 6) {
                octave_bin_counter = 0;
            }
        }
        
        // STAGE 4: Fuzz (if enabled) - Always AGGRESSIVE type
        if (fuzz_enabled) {
            float fuzz_signal = signal;
            
            // Bass boost
            static float bass_lpf = 0.0f;
            const float bass_coeff = 0.05f;
            bass_lpf = bass_lpf + bass_coeff * (fuzz_signal - bass_lpf);
            fuzz_signal = fuzz_signal + (bass_lpf * 0.8f);
            
            // Drive control - combines gain and intensity
            // Map 0-1 knob to gain (1-20x) and intensity (0-1) proportionally
            const float gain = 1.0f + (drive_amount * 19.0f); // 1x to 20x
            const float intensity = drive_amount; // 0 to 1
            fuzz_signal *= gain;
            
            // Fuzz with oversampling - 4x for performance
            std::vector<float> oversampled = Oversampling::upsample(fuzz_signal);
            for (float& sample : oversampled) {
                sample = Fuzz::process(sample, FuzzType::AGGRESSIVE, intensity);
            }
            fuzz_signal = Oversampling::downsample(oversampled);
            
            // Gate control - variable threshold
            if (gate_threshold > 0.01f) {
                static float gate_envelope = 0.0f;
                const float gate_level = gate_threshold * 0.1f; // Scale threshold
                const float gate_attack = 0.95f;
                const float gate_release = 0.01f;
                
                float input_level = std::abs(signal); // Gate detects pre-fuzz level
                
                if (input_level > gate_level) {
                    gate_envelope += gate_attack * (1.0f - gate_envelope);
                } else {
                    gate_envelope += gate_release * (0.0f - gate_envelope);
                }
                
                fuzz_signal *= gate_envelope;
            }
            
            // Tone control
            tone.SetFreq(tone_freq);
            fuzz_signal = tone.Process(fuzz_signal);
            
            signal = fuzz_signal;
        }
        
        // STAGE 5: Autowah AFTER fuzz (if placement is MIDDLE and enabled)
        if (autowah_enabled && autowah_placement == 1) {
            // Envelope detection
            float envelope = envelopeFollower.Process(signal);
            
            // Gate ADSR based on envelope level and threshold
            float gate_level = 0.05f + (autowah_threshold * 0.15f);
            bool gate = (envelope > gate_level);
            float adsr_out = autowah_adsr.Process(gate);
            
            // Map ADSR output to filter frequency with range control
            float range_min = 100.0f + (autowah_range * 200.0f);
            float range_max = 1100.0f + (autowah_range * 1900.0f);
            float filter_freq = range_min + (adsr_out * (range_max - range_min));
            
            autowah_svf.SetFreq(filter_freq);
            autowah_svf.SetRes(0.7f);
            
            // Process through SVF and use bandpass output with gain compensation
            autowah_svf.Process(signal);
            signal = autowah_svf.Band() * 2.0f;  // Autowah makeup gain
        }
        
        // STAGE 6: Autowah AFTER everything (if placement is DOWN and enabled)
        if (autowah_enabled && autowah_placement == 2) {
            // Envelope detection
            float envelope = envelopeFollower.Process(signal);
            
            // Gate ADSR based on envelope level and threshold
            float gate_level = 0.05f + (autowah_threshold * 0.15f);
            bool gate = (envelope > gate_level);
            float adsr_out = autowah_adsr.Process(gate);
            
            // Map ADSR output to filter frequency with range control
            float range_min = 100.0f + (autowah_range * 200.0f);
            float range_max = 1100.0f + (autowah_range * 1900.0f);
            float filter_freq = range_min + (adsr_out * (range_max - range_min));
            
            autowah_svf.SetFreq(filter_freq);
            autowah_svf.SetRes(0.7f);
            
            // Process through SVF and use bandpass output with gain compensation
            autowah_svf.Process(signal);
            signal = autowah_svf.Band() * 2.0f;  // Autowah makeup gain
        }
        
        // STAGE 6.5: FS2 Makeup Gain (NEW FIX)
        // Compensate for volume loss from autowah bandpass and octave processing
        // Only apply when FS2 effects are active and fuzz is bypassed
        if ((autowah_enabled || octave_enabled) && !fuzz_enabled) {
            signal *= 2.0f;  // Makeup gain for FS2 effects
        }
        
        // STAGE 7: Dry/Wet Mix (Knob 2)
        const float mix = knobValues[1];
        signal = input * (1.0f - mix) + signal * mix;
        
        // STAGE 8: Output Level (Knob 3)
        const float level = knobValues[2];
        signal *= level;
        
        out[0][i] = signal;
        out[1][i] = signal;
    }
}

int main(void) {
    // CPU boost to 480MHz for better performance
    hw.Init(true);
    hw.SetAudioBlockSize(256);  // Larger block size for efficiency
    
    float samplerate = hw.AudioSampleRate();
    tone.Init(samplerate);
    
    // Initialize ADSR for autowah envelope
    autowah_adsr.Init(samplerate);
    autowah_adsr.SetAttackTime(0.1f);
    autowah_adsr.SetTime(ADSR_SEG_DECAY, 0.15f);
    autowah_adsr.SetReleaseTime(0.2f);
    autowah_adsr.SetSustainLevel(0.3f);
    
    // Initialize SVF for autowah filter
    autowah_svf.Init(samplerate);
    autowah_svf.SetFreq(800.0f);
    autowah_svf.SetRes(0.7f);
    
    envelopeFollower.Init(samplerate, 5.0f, 50.0f); // Default medium sensitivity
    
    // Initialize octave buffers
    for (int j = 0; j < 6; ++j) {
        octave_buff[j] = 0.0f;
        octave_buff_out[j] = 0.0f;
    }
    octave_bin_counter = 0;
    
    led1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
    led2.Init(hw.seed.GetPin(Hothouse::LED_2), false);
    led1.Update();
    led2.Update();
    
    // Initialize control values
    for(int i = 0; i < 6; i++) {
        knobValues[i] = 0.5f;
        prevKnobValues[i] = 0.5f;
    }
    for(int i = 0; i < 3; i++) {
        toggleValues[i] = 1;
        prev_toggleValues[i] = -1;
    }
    
    // Initialize effect parameters
    drive_amount = 0.5f;
    tone_freq = 800.0f;
    gate_threshold = 0.0f;
    autowah_speed = 0.5f;
    autowah_threshold = 0.5f;
    autowah_range = 0.5f;
    octave_up_level = 0.5f;
    octave_down_level = 0.5f;
    octave_mix = 0.5f;
    
    fuzz_enabled = false;
    autowah_enabled = false;
    octave_enabled = false;
    autowah_placement = 0;
    first_start = true;
    
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while(1)
    {
        if(hw.switches[Hothouse::FOOTSWITCH_1].TimeHeldMs() >= 2000)
        {
            hw.StopAudio();
            hw.StopAdc();
            
            for(int i = 0; i < 3; i++) 
            {
                led1.Set(1.0f);
                led2.Set(0.0f);
                led1.Update();
                led2.Update();
                System::Delay(100);
                
                led1.Set(0.0f);
                led2.Set(1.0f);
                led1.Update();
                led2.Update();
                System::Delay(100);
            }
            
            System::ResetToBootloader();
        }
        
        System::Delay(100);
    }
}
