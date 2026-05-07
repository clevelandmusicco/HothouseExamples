# TriPhase Theorem

Sorta authentic digital models of three iconic 1970s analog phase shifter pedals in a single effect, switchable in real time:

- **EHX Small Stone:** 4-stage OTA phaser with exponential frequency response and COLOR feedback mode
- **MXR Phase 90 (Script version):** 4-stage JFET phaser with the warmer, feedback-free "script logo" voicing
- **MXR Phase 45:** 2-stage JFET phaser; subtle, transparent, "woody" single-notch sweep

After crunching numbers and translating to DSP, the models sounded pretty good, but the Phase 45/90 models needed some tweaks here and there to sound "right", so don't expect 100% traceability back to the schematics. I don't remember precisely _what_ was tweaked, however ... it was a rainy day and there may have been Irish whiskey involved 🥃

## How It Works

All three pedals share the same fundamental architecture: a cascade of first-order all-pass filters whose break frequencies are swept by an LFO. Mixing the phase-shifted signal 50/50 with the dry signal produces **notches** (narrow frequency dips) wherever the accumulated phase shift equals an odd multiple of 180°.

The number of stages determines how many notches appear and where:

- **2 stages** (Phase 45): one notch, located at exactly the all-pass break frequency `fc`
- **4 stages** (Phase 90, Small Stone): two notches, at approximately `0.414·fc` and `2.414·fc`

The dramatic differences in character between the pedals come from:

1. **LFO waveform:** Phase 45/90 use a triangle wave; Small Stone uses a sine wave
2. **Frequency mapping:** Phase 45/90 map LFO linearly to `fc` (JFET behavior); Small Stone maps LFO *exponentially* to `fc`, which makes the sweep accelerate through the midrange and stall at the extremes
3. **Feedback (Small Stone COLOR):** Routes the all-pass output back to the input, sharpening notch resonance and creating the "comb filter" mode

## Controls

| Control | Function |
| --- | --- |
| KNOB 1 | **RATE/SPEED:** LFO rate for all three models (logarithmic taper, ~0.05 Hz to 10 Hz) |
| KNOB 2 | *(unused)* |
| KNOB 3 | *(unused)* |
| KNOB 4 | *(unused)* |
| KNOB 5 | *(unused)* |
| KNOB 6 | *(unused)* |
| SWITCH 1 UP | Selects **Small Stone** model |
| SWITCH 1 MIDDLE | Selects **Phase 90** (Script) model |
| SWITCH 1 DOWN | Selects **Phase 45** model |
| SWITCH 2 DOWN | Small Stone **COLOR off** (only active when Small Stone is selected) |
| SWITCH 2 MIDDLE or UP | Small Stone **COLOR on** |
| SWITCH 3 | *(unused)* |
| FOOTSWITCH 1 | *(reserved for DFU mode; hold 2 s to enter bootloader)* |
| FOOTSWITCH 2 | **Bypass** toggle |
| LED 1 | Lit when Small Stone is the active model |
| LED 2 | Lit when effect is active (not bypassed) |

## Notes

All three model instances run continuously; their LFO phases and all-pass filter states are preserved independently. Switching between models mid-sweep produces no clicks or phase discontinuities.

The Phase 90 model is based on the **Script Logo** variant (pre-1981), which lacks the R28 feedback resistor of the Block Logo version. This gives the cleaner, warmer phasing that made the original famous.
