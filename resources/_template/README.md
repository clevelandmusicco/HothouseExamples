# @@template_uc

Contributed by @@your_name \<<@@your_email>\>

## Description

A brief description of the effect goes here.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Unused |  |
| KNOB 2 | Unused |  |
| KNOB 3 | Unused |  |
| KNOB 4 | Unused |  |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | Unused | **UP** - <br/>**MIDDLE** - <br/>**DOWN** -  |
| SWITCH 2 | Unused | **UP** - <br/>**MIDDLE** - <br/>**DOWN** -  |
| SWITCH 3 | Unused | **UP** - <br/>**MIDDLE** - <br/>**DOWN** -  |
| FOOTSWITCH 1 | Unused |  |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |

### MIDI CC mapping

Every knob, toggleswitch, and footswitch is drivable by MIDI CC over USB, so you'll need a USB host (your computer, a [commercial](https://www.google.com/search?q=USB+midi+host) or [DIY solution](https://youtu.be/N4yUduOqR3M?feature=shared)). Channel is omni.

| CONTROL | CC # | PARAM |
|-|-|-|
| KNOB 1 | 14 |  |
| KNOB 2 | 15 |  |
| KNOB 3 | 16 |  |
| KNOB 4 | 17 |  |
| KNOB 5 | 18 |  |
| KNOB 6 | 19 |  |
| SWITCH 1 | 20 | CC 0-42 UP, 43-85 MIDDLE, 86-127 DOWN |
| SWITCH 2 | 21 | CC 0-42 UP, 43-85 MIDDLE, 86-127 DOWN |
| SWITCH 3 | 22 | CC 0-42 UP, 43-85 MIDDLE, 86-127 DOWN |
| FOOTSWITCH 1 | 23 | Momentary press, >= 64 down |
| FOOTSWITCH 2 | 24 | Momentary press, >= 64 down |
| (bypass) | 25 | >= 64 engaged, < 64 bypassed |

CC 23/24 model a *press* of the footswitch, so they do whatever that footswitch does in this effect. CC 25 sets whole-pedal bypass outright, no matter what the footswitches are wired to. Both exist on purpose: the momentary CCs mirror the pedal, the absolute CC is how a host keeps state in sync after a preset change or a reconnect. Set your controller to momentary (not latching) if a footswitch here is a hold-to-do-something function.

Knobs and toggles use hard takeover: a CC overrides the physical control until you move the pot or flip the switch, at which point the physical control reclaims it. No pickup/soft-takeover, so values can jump on reclaim. The DFU-reset gesture is deliberately unreachable over MIDI.
