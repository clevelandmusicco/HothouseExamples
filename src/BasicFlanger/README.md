# BasicFlanger

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A literal port of the [petal/flanger example from DaisyExamples](https://github.com/electro-smith/DaisyExamples/tree/master/petal/flanger). Knobs, switches, and pins are directly accessed without enums, so look at `hothouse.h` if you want to decipher the mappings. Parameters are not used; the `.Process()` function on knobs defaults to a 0.0f -> 1.0f range. And, yes, that's a bitwise XOR being used to toggle the bypass state.

> [!WARNING]
> It's probably fair to call this code 'obfuscated', but it's been left as-is. Sounds great in any case.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | WET | Mixes wet signal with dry |
| KNOB 2 | DELAY |  |
| KNOB 3 | FEEDBACK |  |
| KNOB 4 | FREQ |  |
| KNOB 5 | DEPTH |  |
| KNOB 6 | VOLUME | Global output volume; useful at certain settings where volume drops due to phasing |
| SWITCH 1 | Unused |  |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to "flashable mode" (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
