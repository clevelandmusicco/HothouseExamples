# Flick

Contributed by Boyd Timothy, GitHub: [joulupukki](https://github.com/joulupukki)

This is a reverb, tremolo, and delay pedal. The original goal of this pedal was to displace the Strymon Flint (Reverb and Tremolo) and a delay pedal to save space on a small pedal board.

### Effects

**Platerra Reverb:** This is a plate reverb based on the Dattorro reverb.

**Tremolo:** Tremolo with smooth sine wave, triangle, and square wave settings.

**Delay:** Basic digital delay.

### Demo

Updated demo video (6 January 2025):

[![Demo Video](https://img.youtube.com/vi/RR4Hccq0VbE/0.jpg)](https://www.youtube.com/watch?v=RR4Hccq0VbE)

### Controls (Normal Mode)

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb Dry/Wet Amount |  |
| KNOB 2 | Tremolo Speed |  |
| KNOB 3 | Tremolo Depth |  |
| KNOB 4 | Delay Time |  |
| KNOB 5 | Delay Feedback |  |
| KNOB 6 | Delay Dry/Wet Amount |  |
| SWITCH 1 | Reverb knob funcion | **UP** - 0% Dry, 0-100% Wet<br/>**MIDDLE** - Dry/Wet Mix<br/>**DOWN** - 100% Dry, 0-100% Wet |
| SWITCH 2 | Tremolo Waveform | **UP** - Square<br/>**MIDDLE** - Triangle<br/>**DOWN** - Sine<br/>*Square wave currently clicks and this is a [known bug](https://github.com/joulupukki/hothouse-effects/issues/9).* |
| SWITCH 3 | Trem & Delay Makeup Gain | **UP** - Plus<br/>**MIDDLE** - Normal<br/>**DOWN** - None |
| FOOTSWITCH 1 | Reverb On/Off | Normal press toggles reverb on/off.<br/>Double press toggles reverb edit mode (see below).<br/>Long press for DFU mode. |
| FOOTSWITCH 2 | Delay/Tremolo On/Off | Normal press toggles delay.<br/>Double press toggles tremolo.<br/><br/>**LED:**<br/>- 100% when only relay is active<br/>- 40% pulsing when only tremolo is active<br/>- 100% pulsing when both are active |

### Controls (Reverb Edit Mode)
*Both LEDs flash when in edit mode.*

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Reverb Amount (Wet) | Not saved. Just here for convenience. |
| KNOB 2 | Pre Delay | 0 for Off, up to 0.25 |
| KNOB 3 | Decay |  |
| KNOB 4 | Tank Diffusion |  |
| KNOB 5 | Input High Cutoff Frequency |  |
| KNOB 6 | Tank High Cutoff Frequency |  |
| SWITCH 1 | Tank Mod Speed | **UP** - High<br/>**MIDDLE** - Medium<br/>**DOWN** - Low |
| SWITCH 2 | Tank Mod Depth | **UP** - High<br/>**MIDDLE** - Medium<br/>**DOWN** - Low |
| SWITCH 3 | Tank Mod Shape | **UP** - High<br/>**MIDDLE** - Medium<br/>**DOWN** - Low |
| FOOTSWITCH 1 | **CANEL** & Exit | Discards parameter changes and exits Reverb Edit Mode.<br/>Long press for DFU mode. |
| FOOTSWITCH 2 | **SAVE** & Exit | Saves all parameters and exits Reverb Edit Mode. |

### Factory Reset (Restore default reverb parameters)

To enter factory reset mode, **press and hold** **Footswitch #2** when powering the pedal. The LED lights will alternatively blink slowly.

1. Rotate Knob #1 to 100%. The LEDs will quickly flash simultaneously and start blinking faster.
2. Rotate Knob #1 to 0%. The LEDs will quickly flash simultaneously and start blinking faster.
3. Rotate Knob #1 to 100%. The LEDs will quickly flash simultaneously and start blinking faster.
4. Rotate Knob #1 to 0%. The LEDs will quickly flash simultaneously, defaults will be restored, and the pedal will resume normal pedal mode.

To exit factory reset mode without resetting. Power off the pedal and power it back on.