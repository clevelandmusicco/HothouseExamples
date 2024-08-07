# ShimmerVerb

Contributed by Cleveland Music Co.

## Description

A shimmer reverb effect for the Cleveland Music Co. Hothouse DIY DSP platform which uses only the DaisySP library classes.

Creating such an effect involves combining traditional reverb with pitch shifting, typically by an octave. This ShimmerVerb project adds:

* Three pitch shifting intervals for different shimmer flavors
* Chorus-modulated reverb
* Selectable pre-reverb LPF and HPF settings (the so-called Abbey Road reverb trick)
* Subtle post-reverb delay settings
* A "100% wet" mode

As with the Hothouse hardware, the code is suitable for full range instruments like synths, drums, and bass guitar.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb Decay | Range is ~300ms to almost infinity. |
| KNOB 2 | Reverb Dampening | Sets the frequency of a LPF ranging from 600Hz to 22kHz to the wet reverb signal (counter-clockwise for lower frequencies ). |
| KNOB 3 | Reverb Send | Controls level of the input signal sent to the reverb; mimics an effect send level control on a mixing console. |
| KNOB 4 | Modulation Depth | Varies the depth of modulation applied to the wet reverb signal; fully counter-clockwise defeats modulation entirely. |
| KNOB 5 | Modulation Rate | Varies the rate of modulation applied to the wet reverb signal; from 0.1Hz up to ~5Hz. |
| KNOB 6 | Shimmer Level | Controls the level of pitch-shift effect applied to the dry signal prior to the Reverb Send control. |
| SWITCH 1 | Pitch shift interval | **UP** - + 1 octave and a 5th<br/>**MIDDLE** - + 1 octave<br/>**DOWN** - + a 5th |
| SWITCH 2 | Pre-send LPF and HPF filter | **UP** - Heavy filtering (6kHz LPF / 600Hz HPF)<br/>**MIDDLE** - Typical "Abbey Road" filtering (10kHz LPF / 600Hz HPF)<br/>**DOWN** - Essentially no filtering (22kHz LPF / 24Hz HPF)<br/><br/>The "Abbey Road reverb trick" is often used to control muddiness or shrillness on certain sources, especially a reverb buss during mixing. |
| SWITCH 3 | Post-reverb delay | **UP** - applies a fixed 300ms delay to the wet reverb signal; the repeats scale with Reverb Decay<br/>**MIDDLE** - applies a fixed 150ms delay with modest repeats<br/>**DOWN** - no delay |
| FOOTSWITCH 1 | Toggles "100% Wet" mode | Defeats the dry signal leaving only the wet signal; also disables the Reverb Send control and sets the send level to 100%.<br/>Useful in certain situations, (E.g., stereo or wet/dry set-ups). |
| FOOTSWITCH 2 | Toggles effect bypass | The bypassed signal is buffered. |
