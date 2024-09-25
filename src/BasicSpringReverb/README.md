# BasicSpringReverb

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A no-frills spring reverb effect ported from the [DaisyExamples Verb petal example](https://github.com/electro-smith/DaisyExamples/tree/master/petal/Verb).

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb DECAY | Also called time or feedback |
| KNOB 2 | Reverb LPF | Counter-clockwise to filter out more high frequencies from the tail |
| KNOB 3 | Reverb SEND | Clockwise to send more of the dry signal to the reverb effect; works like a mixer FX send |
| KNOB 4 | Unused |  |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | Unused |  |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to "flashable mode" (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |

> [!NOTE]
> For some reason, `Hothouse::CheckResetToBootloader()` successfully resets the Daisy Seed, but it doesn't flash the left LED. So, only the right LED will flash prior to the reset. Figuring that out is a low-priority TODO.
