# StereoReverbWithTails

Contributed by Cleveland Music Co. \<code@clevelandmusicco.com\>

## Description

A simple stereo reverb effect based on the DaisySP ReverbSC class. This is a "tails" implementation, meaning if the effect is disabled while processing audio through the reverb effect, the reverb tail is allowed to ring out. While the tail rings out, any new audio input will not be processed through the reverb effect until it is enabled again.

Also note that _SWITCH 1_ selects between mono-to-stereo processing (UP) and true stereo-to-stereo processing (MIDDLE or DOWN). Mono-to-stereo is useful for mono instruments such as vintage synths, guitars, etc.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb DECAY | Also called time or feedback |
| KNOB 2 | Reverb LPF | Counter-clockwise to filter out more high frequencies from the tail |
| KNOB 3 | Reverb SEND | Clockwise to send more of the dry signal to the reverb effect; works like a mixer FX send |
| KNOB 4 | Reverb PRE-DELAY | 0 to 500ms |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | AUDIO MODE | **UP** - mono-to-stereo mode (left input is copied to right)<br/>**MIDDLE** - true stereo-to-stereo mode<br/>**DOWN** - true stereo-to-stereo mode |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to bootloader mode (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
