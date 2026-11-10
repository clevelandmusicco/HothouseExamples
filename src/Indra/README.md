# Indra

Contributed by Jon Philpott \<<jon.philpott@gmail.com>\>

## Description

*FULL DISCLOSURE:* I used Claude Code to port this code over from an existing Pure Data patch that I've been using for some time for live performance. 

This is my take on a granular effect where there are 7 grain players all acting on a 10 second buffer. the grains themselves are triggered by a elementary cellular automaton (Wolfram style, 1-dimension). The interesting part is that the effect works by monitoring the amplitude of your playing, and there's a probability (controlled by knobs 3 and 5) that it will record your current playing into the grain buffer. The result is that you can tweak the probability and amp threshold to have the pedal constantly update based on your playing. The number of grains playing and be dialed in with Knob 2, to bring in different voices (see table below for the voices)

You can get different textures by switching rule set triggers the grains, I've curated a list of rules that work nicely and they're selectable via the toggle switches.

The LEDs show a couple things:
| LED | Description |
|-|-|
| Left | On: effect is enabled. Flashing: a new grain buffer is currently being recorded |
| Right | Blinking: the current cellular automaton clock tempo |


### Voice Table

| voice | speed | direction | bus |
|-|-|-|-|
| A | 1× | forward | L |
| B | 0.5× | forward | R |
| C | 1.5× | forward | L |
| D | 2× | forward | R |
| E | 1× | reverse | L |
| F | 0.5× | reverse | R |
| G | 2× | reverse | L |

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Tempo | 40–200 BPM, quarter notes. LED 2 flashes on each beat. |
| KNOB 2 | Intensity | Sets how many voices may sound. Fully down, only voice A can fire; fully up, all seven can. |
| KNOB 3 | Record probability | Chance that a loud note starts a recording. After a trigger, further triggers are ignored for 10 s. |
| KNOB 4 | Wet level | Dry signal always stays at unity. |
| KNOB 5 | Trigger threshold | −60 to −20 dBFS RMS. The original patch used −35. |
| KNOB 6 | Tone | Output low-pass, 200 Hz–8 kHz. The original patch used 800 Hz (about 10 o'clock). |
| SWITCH 1 | Rule family | **UP** - evolving<br/>**MIDDLE** - chaotic<br/>**DOWN** - rhythmic loops |
| SWITCH 2 | Rule select | Together with SWITCH 3, picks one of 9 rules in the family. All switches in the middle gives rule 30, the original. |
| SWITCH 3 | Rule select | See SWITCH 2 |
| FOOTSWITCH 1 | Effect on/off | Wet signal fades in or out over 1 s. Auto-recording is off while bypassed. LED 1 is lit when on and blinks while recording. |
| FOOTSWITCH 2 | Record / clear | **Tap** - record now, skipping the probability and lockout<br/>**Double-tap** - clear the buffer |

Hold both footswitches for 2 seconds to enter the bootloader.

### Rules

| SWITCH 1 | rules for SWITCH 2 / 3 = M/M, M/U, M/D, U/M, U/U, U/D, D/M, D/U, D/D |
|-|-|
| MIDDLE | 30 86 135 149 45 75 89 101 110 |
| UP | 60 102 153 195 169 225 124 137 193 |
| DOWN | 105 150 90 165 26 154 73 57 41 |

When you change a rule, the current cells carry over. If the pattern dies or freezes, it restarts from the seed (cell 5 alive).
