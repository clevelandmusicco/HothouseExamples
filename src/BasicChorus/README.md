# BasicChorus

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

Ported directly from the ['petal/chorus' example in DaisyExamples](https://github.com/electro-smith/DaisyExamples/tree/master/petal/chorus). Knobs, switches, and pins are directly accessed without enums, so look at `hothouse.h` if you want to decipher the mappings. Parameters are not used; the `.Process()` function on knobs defaults to a 0.0f -> 1.0f range. And, yes, that's a bitwise XOR being used to toggle the bypass state.

> [!WARNING]
> It's probably fair to call this code 'obfuscated', but it's been left as-is. Sounds great in any case.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | WET | Think of it as a mix control |
| KNOB 2 | FREQ | Sets the frequency of the LFO |
| KNOB 3 | DEPTH | Controls the amplitude of the LFO |
| KNOB 4 | DELAY | Sets delay time between dry and processed signal(s) |
| KNOB 5 | FEEDBACK | Controls how much of the processed signal is fed back into the delay line |
| KNOB 6 | VOLUME | Adjusts the overall volume of the output; useful when switching number of voices |
| SWITCH 1 | VOICES | **UP** - L + R chorused signals summed<br/>**MIDDLE** - 1 voice<br/>**DOWN** - 1 voice |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Unused |  |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
