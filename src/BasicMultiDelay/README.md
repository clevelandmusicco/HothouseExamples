# BasicMultiDelay

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A delay effect with three delay lines, each with its own time and feedback parameters. This is useful for creating realistic slap-back effects: set all three delay times to similar values, and do the same with the feedback values. Varying the knobs slightly alters the size and shape of the "room".

This code is a literal port of the [petal/MultiDelay example from DaisyExamples](https://github.com/electro-smith/DaisyExamples/tree/master/petal/MultiDelay). Knobs, switches, and pins are directly accessed without enums, so look at `hothouse.h` if you want to decipher the mappings.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | DELAY 1 TIME |  |
| KNOB 2 | DELAY 2 TIME |  |
| KNOB 3 | DELAY 3 TIME |  |
| KNOB 4 | DELAY 1 FEEDBACK |  |
| KNOB 5 | DELAY 2 FEEDBACK |  |
| KNOB 6 | DELAY 3 FEEDBACK |  |
| SWITCH 1 | WET/DRY MIX | **UP** - 100% WET<br/>**MIDDLE** - 50% WET / 50% DRY<br/>**DOWN** - 66% DRY / 33% WET |
| SWITCH 2 | VOLUME BOOST | Some settings can result in low output volume; use this to compensate<br/><br/>**UP** - HI<br/>**MIDDLE** - LO<br/>**DOWN** - NONE |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to "flashable mode" (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |

## Implementation notes

Since the Hothouse only has 6 knobs and they are all used for the delay parameters, there's no great way to control the WET/DRY mix. So, this code uses SWITCH 1 with predefined mix values:

```cpp
// we're out of knobs; use a toggleswitch with predefined drywet values
static const int drywetValues[] = {100, 50, 33};
drywet = drywetValues[hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_1)];
```

Tweak these values to taste.
