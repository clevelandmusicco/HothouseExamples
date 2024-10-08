# HarmonicTremVerb

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A simple tremolo with harmonic and normal modes, paired with a reverb effect. The two effects are separate and can be used independently, but the tremolo comes after the reverb like in old vintage tube amps.

The SEASICK tremolo mode is a panned stereo version of the HARMONIC mode. In SEASICK, when the high frequency band is at the trough of its wave on one channel, it is at the peak of its wave on the other. The low frequency band is 180-degrees out of phase with the high frequency band, which results in a stereo phase modulation effect. Adjusting the cutoff frequencies can have a dramatic effect on the perceived stereo field.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb SEND |  |
| KNOB 2 | Tremolo RATE |  |
| KNOB 3 | Tremolo DEPTH |  |
| KNOB 4 | Reverb DECAY |  |
| KNOB 5 | Tremolo low-pass cutoff FREQ | Only used if SWITCH 3 is in UP or DOWN position; see notes below |
| KNOB 6 | Tremolo high-pass cutoff FREQ | Only used if SWITCH 3 is in UP or DOWN position; see notes below |
| SWITCH 1 | Reverb low-pass cutoff FREQ | **UP** - 10kHz<br/>**MIDDLE** - 5kHz<br/>**DOWN** - 2500Hz |
| SWITCH 2 | Tremolo WAVEFORM | **UP** - TRI<br/>**MIDDLE** - SIN<br/>**DOWN** - SQUARE (sorta rounded) |
| SWITCH 3 | Tremolo MODE | **UP** - HARMONIC <br/>**MIDDLE** - NORMAL <br/>**DOWN** - SEASICK (panned harmonic) |
| FOOTSWITCH 1 | Reverb BYPASS |  |
| FOOTSWITCH 2 | Tremolo BYPASS |  |

## Implementation notes

Harmonic tremolo is a unique type of modulation effect found in some vintage guitar amplifiers, notably in certain models made by Fender in the 1960s. Unlike standard amplitude tremolo, which modulates the overall volume of the signal, harmonic tremolo splits the audio signal into two frequency bands and modulates them separately. Here's a breakdown of how this circuit typically works:

1. **Signal Splitting** - The incoming audio signal is first split into two different frequency bands: a high-pass filter is used for the higher frequencies and a low-pass filter for the lower frequencies. This separation allows the circuit to treat different parts of the audio spectrum independently. The cutoff frequencies for these two bands are controlled by KNOBS 5 and 6 in this code. The crossover means there is some inherent volume loss to phasing, but harmonic tremolo is a deal with a devil in that way.

2. **Amplitude Modulation** - Each of these frequency bands is then amplitude-modulated at a rate controlled by an oscillator, often a low-frequency oscillator (LFO). The modulation for the high and low frequencies is typically out of phase with each other. For example, when the low frequencies are at a peak in volume, the high frequencies are at a trough, and vice versa.

3. **Recombination** - After modulation, the two signals are recombined and sent to the amplifier's output stage. This process creates a unique phasing or swirling effect, as the out-of-phase amplitude modulation of the different frequency bands interacts and blends back together. The result is a complex and musical effect that can sound like a combination of flanger, phaser, and tremolo.

The code in this project does essentially all of this in a no-frills way, using the DaisySP library for just about everything. The one exception is the addition of the ExtendedOscillator class, which implements the WAVE_SQUARE_ROUNDED wave form to help remove clicks from discontinuities in the audio.
