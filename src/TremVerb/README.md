# TremVerb

Contributed by [tele_player](https://forum.electro-smith.com/u/tele_player/summary) in the [Electrosmith Forums](https://forum.electro-smith.com/t/hothouse-dsp-pedal-kit/5631/14).

## Description

A tremolo reverb effect not terribly unlike a famous digital trem/verb pedal named after a certain city in Michigan. Features a pulsing LED for visual feedback of the tremolo rate and depth.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Tremolo RATE |  |
| KNOB 2 | Tremolo DEPTH |  |
| KNOB 3 | Reverb AMOUNT | The feedback and low-pass filter cutoff frequency are hard-coded in this example |
| KNOB 4 | Unused |  |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | Tremolo wave shape | **UP** - SAW<br/>**MIDDLE** - SIN<br/>**DOWN** - SQUARE |
| SWITCH 2 | Unused | |
| SWITCH 3 | Unused | |
| FOOTSWITCH 1 | Tremolo enable/bypass | When enabled, LED 1 pulses with the rate and depth of the tremolo effect; If held down for 2 seconds, the Daisy Seed will reset to "flashable mode" (DFU) |
| FOOTSWITCH 2 | Reverb enable/bypass |  |

## Modification Ideas

* Use one of the available toggle switches to change the order of effects. In many vintage amps, the tremolo effect is applied *after* the reverb.
* Program available knobs to control the reverb feedback and low-pass filter cutoff frequency.
* The DaisySP implementations of WAVE_SAW and WAVE_SQUARE can be a bit "clicky" in tremolo applications. Implement a waveform optimised for tremolo applications; for example [this](https://forum.electro-smith.com/t/a-rounded-square-waveform/5129/2).
