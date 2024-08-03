# BasicTremolo

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A literal port of the [petal/tremolo example from DaisyExamples](https://github.com/electro-smith/DaisyExamples/tree/master/petal/tremolo). Knobs, switches, and pins are (mostly) directly accessed without enums, so look at `hothouse.h` if you want to decipher the mappings. Parameters are not used; the `.Process()` function on knobs defaults to a 0.0f -> 1.0f range. And, yes, that's a bitwise XOR being used to toggle the bypass state. :neckbeard:

The square wave is a little "clicky" in this example, even using WAVE_POLYBLEP_SQUARE. A more advanced example using an optimized square waveform that minimizes the "clicks" (caused by discontinuities in the audio) is [HarmonicTremVerb](https://github.com/clevelandmusicco/HothouseExamples/tree/main/src/HarmonicTremVerb).

> [!WARNING]
> It's fair to call this code 'obfuscated', but it's been left as-is for ... posterity? Sounds great in any case.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | FREQ |  |
| KNOB 2 | DEPTH |  |
| KNOB 3 | Unused |  |
| KNOB 4 | Unused |  |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | WAVEFORM | **UP** - TRIANGLE<br/>**MIDDLE** - SINE<br/>**DOWN** - SQUARE |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Unused |  |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
