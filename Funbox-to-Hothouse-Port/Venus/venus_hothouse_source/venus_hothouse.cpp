#include <string.h>
#include "daisysp.h"
#include "hothouse.h"

#include <cmath>
#include <complex>
#include "shy_fft.h"
#include "fourier.h"
#include "wave.h"

#define PI 3.1415926535897932384626433832795

using namespace daisy;
using namespace daisysp;
using namespace clevelandmusicco;
using namespace soundmath;

// Use the Hothouse class properly
Hothouse hw;

float samplerate = 32000;
bool bypass = true;
bool freeze = false;
bool first_start = true;

// Hothouse toggle switch positions
Hothouse::ToggleswitchPosition toggle1_pos, toggle2_pos, toggle3_pos;
Hothouse::ToggleswitchPosition prev_toggle1_pos, prev_toggle2_pos, prev_toggle3_pos;

Led led1, led2;

// Effect parameters - matching original defaults and ranges
float vdecay = 10, vmix = 0.5, vdamp = 0.1;
float vshimmer = 0.0, vshimmer_tone = 0.0, vdetune = 0.0;
int shimmer_mode = 0, reverb_mode = 0, drift_mode = 1;  // Original defaults
int detune_mode = 1, detune_multiplier = 1;

// STFT components
const size_t order = 12;
const size_t N = (1 << order);
const float sqrtN = sqrt(N);
const size_t laps = 4;
const size_t buffsize = 2 * laps * N;
float in[buffsize], middle[buffsize], out[buffsize];
float reverb_energy[N/2];

ShyFFT<float, N, RotationPhasor>* fft;
Fourier<float, N>* stft;
Wave<float> hann([] (float phase) -> float { return 0.5 * (1 - cos(2 * PI * phase)); });

// Audio processing objects
SampleRateReducer samplerateReducer;
Tone lowpass;  // Low Pass for lofi mode

// Drift oscillators
Oscillator drift_osc, drift_osc2, drift_osc3, drift_osc4;
float drift_multiplier = 1.0, drift_multiplier2 = 1.0;
float drift_multiplier3 = 1.0, drift_multiplier4 = 1.0;

// Effect calculation variables
float fft_size = N / 2;
float octave_up_rate_persecond, octave_up_rate_perinterval;
float shimmer_double, shimmer_triple, shimmer_remainder;
float detune_rate_persecond, detune_rate_perinterval;
float detune_double, detune_remainder;
float window_samples = 32768;
float interval_samples = ceil(window_samples/laps);

void updateSwitch1()
{
    // CORRECTED MAPPING: Physical DOWN=2, MIDDLE=1, UP=0
    // Original: left=octave down, center=octave up, right=up+down
    if (toggle1_pos == Hothouse::TOGGLESWITCH_DOWN) {      // case 2 = physical DOWN
        shimmer_mode = 0;  // octave down (was left)
    } else if (toggle1_pos == Hothouse::TOGGLESWITCH_MIDDLE) { // case 1 = physical MIDDLE  
        shimmer_mode = 1;  // octave up (was center)
    } else if (toggle1_pos == Hothouse::TOGGLESWITCH_UP) {     // case 0 = physical UP
        shimmer_mode = 2;  // octave up and down (was right)
    }
}

void updateSwitch2()
{
    // Original: left=less lofi, center=normal, right=more lofi
    if (toggle2_pos == Hothouse::TOGGLESWITCH_DOWN) {      // case 2 = physical DOWN
        reverb_mode = 0;  // less lofi
        samplerateReducer.SetFreq(0.3);
        lowpass.SetFreq(8000.0);
    } else if (toggle2_pos == Hothouse::TOGGLESWITCH_MIDDLE) { // case 1 = physical MIDDLE
        reverb_mode = 1;  // normal
    } else if (toggle2_pos == Hothouse::TOGGLESWITCH_UP) {     // case 0 = physical UP
        reverb_mode = 2;  // more lofi
        samplerateReducer.SetFreq(0.2);
    }
}

void updateSwitch3()
{
    // Original: left=slower drift, center=no drift, right=faster drift
    if (toggle3_pos == Hothouse::TOGGLESWITCH_DOWN) {      // case 2 = physical DOWN
        drift_mode = 0;  // slower drift
        drift_osc.SetFreq(0.009);
        drift_osc.SetWaveform(0);  // WAVE_SIN
        drift_osc2.SetFreq(0.01);
        drift_osc2.SetWaveform(0);
        drift_osc3.SetFreq(0.011);
        drift_osc3.SetWaveform(0);
        drift_osc4.SetFreq(0.012);
        drift_osc4.SetWaveform(0);
    } else if (toggle3_pos == Hothouse::TOGGLESWITCH_MIDDLE) { // case 1 = physical MIDDLE
        drift_mode = 1;  // no drift
    } else if (toggle3_pos == Hothouse::TOGGLESWITCH_UP) {     // case 0 = physical UP
        drift_mode = 2;  // faster drift
        drift_osc.SetFreq(0.020);
        drift_osc.SetWaveform(1);  // WAVE_TRI
        drift_osc2.SetFreq(0.025);
        drift_osc2.SetWaveform(1);
        drift_osc3.SetFreq(0.03);
        drift_osc3.SetWaveform(1);
        drift_osc4.SetFreq(0.035);
        drift_osc4.SetWaveform(1);
    }
}

void ProcessControls()
{
    hw.ProcessAllControls();
    
    // Process footswitches - matching original behavior
    if(hw.switches[Hothouse::FOOTSWITCH_1].RisingEdge()) {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }
    
    // Freeze on footswitch 2 press (momentary)
    if (hw.switches[Hothouse::FOOTSWITCH_2].Pressed()) {
        freeze = true;
    } else {
        freeze = false;
    }
    led2.Set(freeze ? 1.0f : 0.0f);
    
    // Process toggle switches
    toggle1_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1);
    toggle2_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_2);
    toggle3_pos = hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_3);
    
    if (toggle1_pos != prev_toggle1_pos || first_start) {
        updateSwitch1();
        prev_toggle1_pos = toggle1_pos;
    }
    
    if (toggle2_pos != prev_toggle2_pos || first_start) {
        updateSwitch2();
        prev_toggle2_pos = toggle2_pos;
    }
    
    if (toggle3_pos != prev_toggle3_pos || first_start) {
        updateSwitch3();
        prev_toggle3_pos = toggle3_pos;
    }
    
    first_start = false;
    
    // Process knobs with correct parameter curves
    float knobValues[6];
    knobValues[0] = hw.GetKnobValue(Hothouse::KNOB_1);  // Linear for decay
    knobValues[1] = hw.GetKnobValue(Hothouse::KNOB_2);  // Linear for mix
    
    // CRITICAL: Apply exponential curve to damp (matches original Parameter::EXPONENTIAL)
    float raw_damp = hw.GetKnobValue(Hothouse::KNOB_3);
    knobValues[2] = powf(raw_damp, 2.0f);  // Exponential approximation
    
    knobValues[3] = hw.GetKnobValue(Hothouse::KNOB_4);  // Linear for shimmer
    knobValues[4] = hw.GetKnobValue(Hothouse::KNOB_5);  // Linear for shimmer_tone
    knobValues[5] = hw.GetKnobValue(Hothouse::KNOB_6);  // Linear for detune
    
    // Apply correct scaling to match original Parameter ranges
    vdecay = knobValues[0] * 99 + 1;           // 1-100
    vmix = knobValues[1];                      // 0-1
    vdamp = knobValues[2];                     // 0-1 (with exponential curve applied)
    vshimmer = knobValues[3] * 0.1f;           // 0-0.1
    vshimmer_tone = knobValues[4] * 0.3f;      // 0-0.3
    
    // Correct detune range: -0.15 to +0.15 (matching original)
    float vdetune_temp = (knobValues[5] - 0.5f) * 0.3f;  // Maps 0-1 to -0.15 to +0.15
    vdetune = abs(vdetune_temp);  // Take absolute value like original
    
    // Apply drift automation (matching original exactly)
    if (drift_mode == 0 || drift_mode == 2) {
        vdamp = vdamp * abs(drift_multiplier) * 0.7f + 0.3f;
        vshimmer *= abs(drift_multiplier2);
        vshimmer_tone *= abs(drift_multiplier3);
        vdetune *= abs(drift_multiplier4);  // If detune set to noon, this should have no effect
    }
    
    // Calculate detune mode (matching original logic exactly)
    if (vdetune > 0.03f) {  // gives a 10% knob range at noon for no detuning
        vdetune = vdetune - 0.029f;  // account for reduced range
        if (vdetune_temp >= 0) {
            detune_mode = 2;  // detune up
            detune_multiplier = 1;
        } else {
            detune_mode = 0;  // detune down
            detune_multiplier = -1;
        }
    } else {
        detune_mode = 1;  // no detuning
    }
    
    // Calculate shimmer parameters (exact original formulas)
    octave_up_rate_persecond = std::pow(8.0f, vshimmer) - 1;
    octave_up_rate_perinterval = std::min(0.75f, octave_up_rate_persecond/samplerate*interval_samples);
    
    // Make 5ths independent of shimmer control
    float octave_up_rate_persecond2 = std::pow(8.0f, vshimmer_tone) - 1;
    float octave_up_rate_perinterval2 = std::min(0.75f, octave_up_rate_persecond2/samplerate*interval_samples);
    
    shimmer_double = octave_up_rate_perinterval*(1 - vshimmer_tone/1.58f);
    shimmer_triple = (octave_up_rate_perinterval2/1.58f) * vshimmer_tone;
    shimmer_remainder = (1 - shimmer_double - shimmer_triple);
    
    detune_rate_persecond = std::pow(8.0f, vdetune) - 1;
    detune_rate_perinterval = std::min(0.75f, detune_rate_persecond/samplerate*interval_samples);
    detune_double = detune_rate_perinterval;
    detune_remainder = 1 - detune_double;
}

void AudioCallback(AudioHandle::InputBuffer in_buf, AudioHandle::OutputBuffer out_buf, size_t size)
{
    // Update LEDs at start of callback (matching original)
    led1.Update();
    led2.Update();
    
    ProcessControls();
    
    for(size_t i = 0; i < size; i++) {
        // Process drift oscillators
        drift_multiplier = drift_osc.Process();
        drift_multiplier2 = drift_osc2.Process();
        drift_multiplier3 = drift_osc3.Process();
        drift_multiplier4 = drift_osc4.Process();
        
        if(bypass) {
            out_buf[0][i] = in_buf[0][i];
            out_buf[1][i] = in_buf[1][i];
        } else {
            stft->write(in_buf[0][i]);  // put a new sample in the STFT
            
            float wet = 0.0;
            if (reverb_mode == 0) {  // less lofi
                wet = lowpass.Process(samplerateReducer.Process(stft->read()));
            } else if (reverb_mode == 1) {  // normal
                wet = stft->read();
            } else if (reverb_mode == 2) {  // more lofi
                wet = samplerateReducer.Process(stft->read());
            }
            
            // Mix wet and dry signals
            out_buf[0][i] = wet * vmix + in_buf[0][i] * (1.0f - vmix);
            out_buf[1][i] = out_buf[0][i];  // Mono processing
        }
    }
}

// Reverb processing function
inline void reverb(const float* in_freq, float* out_freq)
{
    // convenient constant for grabbing imaginary parts
    static const size_t offset = N / 2;
    
    for (size_t i = 0; i < N / 2; i++) {
        float fft_bin = i + 1;
        float real = in_freq[i];
        float imag = in_freq[i + offset];
        float energy = real * real + imag * imag;
        
        // Amplitude from energy
        float reverb_amp = sqrt(reverb_energy[i]);
        if (fft_bin / fft_size > vdamp) {
            // Reduce amplitude by 1/f
            reverb_amp *= vdamp * fft_size/fft_bin;
        }
        
        // Add random phase reverb energy
        // CRITICAL: Match original - no division by RAND_MAX
        float random_phase = rand()*2*PI;
        real = reverb_amp * cos(random_phase);
        imag = reverb_amp * sin(random_phase);
        
        // If frozen, don't add new energy or decay the reverb
        if (!freeze) {
            // Add current energy to reverb
            reverb_energy[i] += energy / laps;  // laps=4 "overlap factor"
            
            // Decay reverb
            float reverb_decay_factor = 1.0f/vdecay;
            reverb_energy[i] *= 1.0f - reverb_decay_factor;
            
            float half_fft_size = fft_size/2;
            float current = reverb_energy[i];
            
            if (i > 0 && i < half_fft_size - 2) {  // Prevents accessing outside of array index
                // Morph reverb up by octaves up or down
                if (shimmer_mode == 1 || shimmer_mode == 2) {  // up octave
                    reverb_energy[2*i - 1] += 0.123f*shimmer_double*current;
                    reverb_energy[2*i] += 0.25f*shimmer_double*current;
                    reverb_energy[2*i + 1] += 0.123f*shimmer_double*current;
                } else if ((shimmer_mode == 0 || shimmer_mode == 2) && i > 1 && !(i % 2)) {  // down octave
                    reverb_energy[i/2 - 1] += 0.75f*shimmer_double*current;
                    reverb_energy[i/2] += 1.5f*shimmer_double*current;
                    reverb_energy[i/2 + 1] += 0.75f*shimmer_double*current;
                }
                
                // Morph reverb up by octave+5th
                if (3*i + 1 < half_fft_size) {
                    reverb_energy[3*i - 2] += 0.055f*shimmer_triple*current;
                    reverb_energy[3*i - 1] += 0.11f*shimmer_triple*current;
                    reverb_energy[3*i] += 0.17f*shimmer_triple*current;
                    reverb_energy[3*i + 1] += 0.11f*shimmer_triple*current;
                    reverb_energy[3*i + 2] += 0.105f*shimmer_triple*current;
                }
                
                // Detune up or down based on detune knob
                if (i > 2 && i < half_fft_size - 2 && detune_mode != 1) {
                    reverb_energy[i + (3*detune_multiplier)] += 0.123f*detune_double*current;
                    reverb_energy[i + (2*detune_multiplier)] += 0.25f*detune_double*current;
                    reverb_energy[i + (1*detune_multiplier)] += 0.123f*detune_double*current;
                }
            }
            
            // Apply remainder factors
            if (detune_mode == 1)
                detune_remainder = 1;
            reverb_energy[i] = detune_remainder * shimmer_remainder * current;
        }
        
        out_freq[i] = real;
        out_freq[i + offset] = imag;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
    samplerate = hw.AudioSampleRate();
    hw.SetAudioBlockSize(256);  // Matching original
    
    // Initialize reverb energy array
    for (size_t i = 0; i < N / 2; i++) {
        reverb_energy[i] = 0.0;
    }
    
    // Initialize toggle positions to unknown
    prev_toggle1_pos = Hothouse::TOGGLESWITCH_UNKNOWN;
    prev_toggle2_pos = Hothouse::TOGGLESWITCH_UNKNOWN;
    prev_toggle3_pos = Hothouse::TOGGLESWITCH_UNKNOWN;
    
    // Initialize LEDs using Hothouse's seed
    led1.Init(hw.seed.GetPin(Hothouse::LED_1), false);
    led2.Init(hw.seed.GetPin(Hothouse::LED_2), false);
    led1.Update();
    led2.Update();
    
    // Set initial bypass state
    bypass = true;
    
    // Initialize FFT and STFT objects
    fft = new ShyFFT<float, N, RotationPhasor>();
    fft->Init();
    stft = new Fourier<float, N>(reverb, fft, &hann, laps, in, middle, out);
    
    // Initialize audio processing objects
    samplerateReducer.Init();
    samplerateReducer.SetFreq(0.3);
    lowpass.Init(samplerate);
    lowpass.SetFreq(8000.0);
    
    // Initialize drift oscillators
    drift_osc.Init(samplerate);
    drift_osc.SetAmp(1.0);
    drift_osc2.Init(samplerate);
    drift_osc2.SetAmp(1.0);
    drift_osc3.Init(samplerate);
    drift_osc3.SetAmp(1.0);
    drift_osc4.Init(samplerate);
    drift_osc4.SetAmp(1.0);
    
    // Set initial parameter values to match original
    vdecay = 10;
    vmix = 0.5;
    vdamp = 0.1;
    vshimmer = 0.0;
    vshimmer_tone = 0.0;
    vdetune = 0.0;
    
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while(1) {
        // Check for Hothouse built-in DFU entry method
        hw.CheckResetToBootloader();
        
        hw.DelayMs(1);
    }
    
    delete stft;
    delete fft;
}