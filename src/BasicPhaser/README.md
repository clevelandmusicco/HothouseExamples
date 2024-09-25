# BasicPhaser

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A literal port of the [petal/phaser example from DaisyExamples](https://github.com/electro-smith/DaisyExamples/tree/master/petal/phaser). Knobs, switches, and pins are (mostly) directly accessed without enums, so look at `hothouse.h` if you want to decipher the mappings. Parameters are not used; the `.Process()` function on knobs defaults to a 0.0f -> 1.0f range. And, yes, that's a bitwise XOR being used to toggle the bypass state.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | WET | Blends wet signal with dry |
| KNOB 2 | LFO FREQ | Sets the frequency of all stages |
| KNOB 3 | LFO DEPTH | Sets all channels' LFO depths |
| KNOB 4 | ALLPASS FREQ | Sets all channels' allpass frequency |
| KNOB 5 | FEEDBACK | Sets all channels' feedback |
| KNOB 6 | STAGES | Controls number of allpass stages (poles) from 1 to 8 |
| SWITCH 1 | Unused |  |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to "flashable mode" (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |

## Implementation notes

The encoder logic in the original example:

```cpp
  //encoder
  numstages += hw.encoder.Increment();
  numstages = DSY_CLAMP(numstages, 1, 8);
  numstages = hw.encoder.RisingEdge() ? 4 : numstages;
  phaser.SetPoles(numstages);
```

is replaced with a standard potentiometer:

```cpp
  // simulate encoder in original example
  phaser.SetPoles(1 + static_cast<int>(hw.knobs[5].Process() * 7.0f));
```
