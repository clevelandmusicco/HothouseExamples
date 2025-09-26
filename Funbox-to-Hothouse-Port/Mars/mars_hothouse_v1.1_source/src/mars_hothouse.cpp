#include "daisy_seed.h"
#include "daisysp.h"
#include "hothouse.h"
#include <RTNeural/RTNeural.h>

// Include the Mars-specific headers that define the types
#include "delayline_2tap.h"
#include "all_model_data_gru9_4count.h"
#include "ImpulseResponse/ImpulseResponse.h"
#include "ImpulseResponse/ir_data.h"

using namespace daisy;
using namespace daisysp;
using namespace clevelandmusicco;

/*
 * Mars Hothouse v1.1 - Neural Amp Modeler with IR and Delay
 * 
 * Changes in v1.1 (September 23, 2025):
 * - Footswitch 2 now controls delay enable/disable
 * - LED 2 indicates delay status
 * - Delay range optimized to 50ms-1s with linear scaling
 * - Reduced delay buffer from 2 seconds to 1 second
 * - Increased output level by 25%
 * - Fixed IR processing and restored original amp models
 * 
 * Original Mars by Keith Bloemer (GuitarML)
 * Hothouse port by Chris Brandt
 */

// Hardware object - using Hothouse library
Hothouse hw;

// Audio processing objects
Tone tone;        // Low Pass
ATone toneHP;     // High Pass - ATone like original Mars.cpp
Balance bal;      // Balance for volume correction in filtering

// Delay Max Definitions (Assumes 48kHz samplerate)
#define MAX_DELAY static_cast<size_t>(48000.0f * 1.f)  // MODIFIED: 1 second max delay
// Original 2 second delay: #define MAX_DELAY static_cast<size_t>(48000.0f * 2.f)
DelayLine2Tap<float, MAX_DELAY> DSY_SDRAM_BSS delayLine;

// Impulse Response - REPLICATED from original Mars
ImpulseResponse mIR;
int m_currentIRindex;

// Control variables
float knobValues[6] = {0.0f};
int toggleValues[3] = {1};
int prev_toggleValues[3] = {-1, -1, -1};
bool dipValues[4] = {true, true, false, false};

// Effect parameters
float nnLevelAdjust = 1.0f;
float mix_effects = 0.5f;
int blink = 0;
bool trigger_save = false;
bool bypass = true;
bool delay_bypassed = true;  // NEW: FS2 latching delay enable
bool first_start = true;

// LEDs
Led led1, led2;

// Enhanced delay structure with 2-tap capability - BASED ON original Mars (modified for 1-second buffer)
struct delay
{
    DelayLine2Tap<float, MAX_DELAY> *del;
    float                        currentDelay;
    float                        delayTarget;
    float                        feedback = 0.0;
    float                        active = false;
    float                        level = 1.0;      // Level multiplier of output
    bool                         secondTapOn = false;
    
    float Process(float in)
    {
        //set delay times
        fonepole(currentDelay, delayTarget, .0002f);
        del->SetDelay(currentDelay);

        float read = del->Read();

        float secondTap = 0.0;
        if (secondTapOn) {
            secondTap = del->ReadSecondTap();
        }

        if (active) {
            del->Write((feedback * read) + in);
        } else {
            del->Write(feedback * read); // if not active, don't write any new sound to buffer
        }

        return (read + secondTap) * level;

    }
};

delay delay1;

// Neural Network Model - Real RTNeural implementation
RTNeural::ModelT<float, 1, 1,
    RTNeural::GRULayerT<float, 1, 9>,
    RTNeural::DenseT<float, 9, 1>> model;

// Neural model selection - RESTORED ORIGINAL from Mars.cpp with Hothouse switch mapping
void updateSwitch1() 
{
    // Hothouse switch mapping: 0=UP, 1=MIDDLE, 2=DOWN
    // Original Mars used toggleValues[0] + 1 for model index
    int modelIndex = toggleValues[0] + 1;

    auto& gru = (model).template get<0>();
    auto& dense = (model).template get<1>();
    gru.setWVals(model_collection[modelIndex].rec_weight_ih_l0);
    gru.setUVals(model_collection[modelIndex].rec_weight_hh_l0);
    gru.setBVals(model_collection[modelIndex].rec_bias);
    dense.setWeights(model_collection[modelIndex].lin_weight);
    dense.setBias(model_collection[modelIndex].lin_bias.data());
    model.reset();

    // RESTORED: Original model level adjust without test multipliers
    nnLevelAdjust = model_collection[modelIndex].levelAdjust;
}

// REPLICATED EXACTLY from original Mars
void updateSwitch2() 
{
    int irIndex = toggleValues[1];
    mIR.Init(ir_collection[irIndex]);  // ir_data is from ir_data.h
}

// REPLICATED EXACTLY from original Mars
void updateSwitch3() 
{
    if (toggleValues[2] == 0) {
        delay1.secondTapOn = false;

    } else if (toggleValues[2] == 2) {
        delay1.secondTapOn = true;
        delay1.del->set2ndTapFraction(0.6666667); // triplett

    } else {
        delay1.secondTapOn = true;
        delay1.del->set2ndTapFraction(0.75); // dotted eighth

    }
}

void UpdateLEDs() {
    led1.Set(bypass ? 0.0f : 1.0f);
    led2.Set(delay_bypassed ? 0.0f : 1.0f);  // NEW: Show delay state
    led1.Update();
    led2.Update();
}

void ProcessControls() {
    hw.ProcessAllControls();
    
    // Read knobs using Hothouse interface
    knobValues[0] = hw.GetKnobValue(Hothouse::KNOB_1);  // Gain
    knobValues[1] = hw.GetKnobValue(Hothouse::KNOB_2);  // Mix
    knobValues[2] = hw.GetKnobValue(Hothouse::KNOB_3);  // Level
    
    // Apply cubic curve to filter knob for more natural response like original Mars.cpp
    float raw_filter = hw.GetKnobValue(Hothouse::KNOB_4);
    knobValues[3] = raw_filter * raw_filter * raw_filter; // Cubic curve
    
    knobValues[4] = hw.GetKnobValue(Hothouse::KNOB_5);  // Delay Time
    knobValues[5] = hw.GetKnobValue(Hothouse::KNOB_6);  // Delay Feedback
    
    // Read toggle switches and update models if changed
    int newToggle1 = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
    int newToggle2 = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
    int newToggle3 = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
    
    if (newToggle1 != toggleValues[0] || first_start) {
        toggleValues[0] = newToggle1;
        updateSwitch1(); // Update neural model
    }
    
    if (newToggle2 != toggleValues[1] || first_start) {
        toggleValues[1] = newToggle2;
        updateSwitch2(); // Update IR
    }
    
    if (newToggle3 != toggleValues[2] || first_start) {
        toggleValues[2] = newToggle3;
        updateSwitch3(); // Update delay mode
    }
    
    first_start = false;
    
    // MODIFIED delay control logic - NEW: FS2 controls enable only
    // delay is active when FS2 enables it (no knob threshold check)
    delay1.active = !delay_bypassed;

    // MODIFIED: Linear scaling from 50ms to 1 second (2400 to 48000 samples)
    delay1.delayTarget = 2400 + knobValues[4] * 45600; // 50ms to 1000ms range
    
    /* Original 2-second delay code with two-range system:
    // From 0 to 75% knob is 0 to 1 second, 75% to 100% knob is 1 to 2 seconds
    if (knobValues[4] <= 0.75) {
        delay1.delayTarget = 2400 + knobValues[4] * 60800; // in samples 50ms to 1 second range
    } else {
        delay1.delayTarget = 48000 + (knobValues[4] - 0.75) * 192000; // 1 second to 2 second range
    }
    */
    
    /* Original Mars delay threshold logic (turned off when knob < 1%):
    if (knobValues[4] < 0.01) {   // if knob < 1%, set delay to inactive
        delay1.active = false;
    } else {
        delay1.active = true;
    }
    */
    
    delay1.feedback = knobValues[5];
    
    mix_effects = knobValues[1];
    
    // Read footswitches - PRESERVE EXACT FS1 functionality from working mars_hothouse.cpp
    if (hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
        bypass = !bypass;
    }
    
    // NEW: FS2 as delay enable/disable latch
    if (hw.switches[Hothouse::FOOTSWITCH_2].RisingEdge()) {
        delay_bypassed = !delay_bypassed;
    }
    
    UpdateLEDs();
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    ProcessControls();
    
    // Calculate mix parameters - Modified for more gradual transition
    // Apply curve to make wet signal come in more gradually
    float curved_mix = mix_effects * mix_effects; // Square the mix for more gradual wet transition
    
    float x2 = 1.0 - curved_mix;
    float A = curved_mix*x2;
    float B = A * (1.0 + 1.4186 * A);
    float C = B + curved_mix;
    float D = B + x2;

    float wetMix = C * C;
    float dryMix = D * D;
    
    for (size_t i = 0; i < size; i++) {
        float input = in[0][i];
        float dry_signal = input; // Store clean dry signal
        float wet_signal = input; // Process wet signal separately
        
        if (!bypass) {
            // RESTORED: Original Mars.cpp baseline gain range (0.1 to 2.5)
            float vgain = knobValues[0] * 2.4f + 0.1f; // Convert 0.0-1.0 to 0.1-2.5 range
            float input_arr[1] = {wet_signal * vgain};
            
            if (dipValues[0]) { // Neural model enabled
                wet_signal = model.forward(input_arr) + input_arr[0]; // Add clean signal
                wet_signal *= nnLevelAdjust; // RESTORED: Simple level adjust from model
            } else {
                wet_signal = input_arr[0];
            }
            
            // ORIGINAL MARS.CPP FILTER PROCESSING - exactly like the original
            float filter_in = wet_signal;
            float filter_out;
            float balanced_out;
            
            float vfilter = knobValues[3]; // Use the cubic-curved filter value
            if (vfilter <= 0.5f) {
                // Low pass mode: 100Hz to ~20kHz like original
                float filter_value = (vfilter * 39800.0f) + 100.0f;
                tone.SetFreq(filter_value);
                filter_out = tone.Process(filter_in);
                balanced_out = bal.Process(filter_out, filter_in);
            } else {
                // High pass mode: 40Hz to 440Hz like original  
                float filter_value = (vfilter - 0.5f) * 800.0f + 40.0f;
                toneHP.SetFreq(filter_value);
                filter_out = toneHP.Process(filter_in);
                balanced_out = bal.Process(filter_out, filter_in);
            }
            
            // EXACT REPLICATION of Mars audio chain: Gain -> Neural Model -> Tone -> Delay -> IR
            float delay_out = delay1.Process(balanced_out);   // Moved delay prior to IR
            
            // IMPULSE RESPONSE - EXACT REPLICATION from original Mars
            float impulse_out = 0.0;
            
            if (dipValues[1]) // If IR is enabled by dip switch
            {
                impulse_out = mIR.Process(balanced_out * dryMix + delay_out * wetMix) * 0.2;  
            } else {
                impulse_out = balanced_out * dryMix + delay_out * wetMix; 
            }
            
            // Output level - MODIFIED: Increased from 0.4 to 0.5 for more output
            float output = impulse_out * knobValues[2] * 0.5f; // Original: 0.4f
            
            out[0][i] = output;
            out[1][i] = output; // Mono to stereo
        } else {
            // Bypass - just pass dry signal through
            out[0][i] = dry_signal;
            out[1][i] = dry_signal;
        }
    }
}

int main(void) {
    // Initialize hardware using Hothouse library
    hw.Init(true); // CPU boost for performance
    
    // Setup model weights FIRST
    setupWeights();
    
    // Initialize audio processing objects
    float samplerate = hw.AudioSampleRate();
    hw.SetAudioBlockSize(256); // Performance optimization from Mars developer
    
    tone.Init(samplerate);      // Low pass
    toneHP.Init(samplerate);    // High pass
    bal.Init(samplerate);       // Balance for volume correction
    
    // Initialize enhanced delay - EXACT REPLICATION from original Mars
    delayLine.Init();
    delay1.del = &delayLine;
    delay1.delayTarget = 2400; // in samples
    delay1.feedback = 0.0;
    delay1.active = true;
    
    // Initialize LEDs
    led1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
    led2.Init(hw.seed.GetPin(Hothouse::LED_2), false);
    led1.Update();
    led2.Update();
    
    // Initialize default settings
    for(int i = 0; i < 6; i++) {
        knobValues[i] = 0.0f;
    }
    for(int i = 0; i < 3; i++) {
        toggleValues[i] = 0; // Start with first position
    }
    for(int i = 0; i < 4; i++) {
        dipValues[i] = true;
    }
    
    // Initialize first neural model and IR
    first_start = true; // Will trigger all switch updates on first ProcessControls call
    
    nnLevelAdjust = 1.0f;
    mix_effects = 0.5f;
    bypass = true;
    delay_bypassed = true; // Start with delay off
    
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while(1) {
        // Settings save functionality
        if(trigger_save) {
            // Save settings functionality - placeholder
            trigger_save = false;
            blink = 0;
        }
        
        // Hothouse DFU entry
        hw.CheckResetToBootloader();
        
        System::Delay(10);
    }
}