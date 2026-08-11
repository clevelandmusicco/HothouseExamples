# BasicSynthCC

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

Experimental fork of BasicSynth demonstrating baked-in MIDI CC support. Same monophonic synth (multiple waveforms, Moog-style filter, resonance, attack, decay), but the four knob-controlled params can also be driven by MIDI CC over USB. Note information (and CC/PC) is read via MIDI over USB, so you'll need a USB host (your computer, a [commercial](https://www.google.com/search?q=USB+midi+host) or [DIY solution](https://youtu.be/N4yUduOqR3M?feature=shared)).

### MIDI CC mapping

`Hothouse::GetKnobValue()` merges the physical pot with the last MIDI CC received for that knob, and whichever moved most recently wins. Knobs use hard takeover (touch-reclaim): turning the pot after a CC has taken over releases the override once the pot has moved again, at which point the physical knob is back in control. There's no pickup/soft-takeover (yet): the value can jump the instant the knob reclaims it. Toggleswitches work the same way, but reclaim on any physical flip. Footswitches are simpler: CC is just OR'd with the physical switch, so either can assert "pressed" and there's nothing to reclaim. CC values are absolute (0-127), not relative encoder deltas.

| CONTROL | CC # | PARAM |
| - | - | - |
| KNOB 1 | 14 | FILTER |
| KNOB 2 | 15 | RESONANCE |
| KNOB 3 | 16 | ATTACK |
| KNOB 4 | 17 | DECAY |
| KNOB 5 | 18 | Unused |
| KNOB 6 | 19 | Unused |
| SWITCH 1 | 20 | Waveform (CC 0-42 SIN, 43-85 SQUARE, 86-127 SAW) |
| SWITCH 2 | 21 | Unused (received, not acted on) |
| SWITCH 3 | 22 | Unused (received, not acted on) |
| FOOTSWITCH 1 | 23 | Unused (received, not acted on) |
| FOOTSWITCH 2 | 24 | Unused (received, not acted on) |
| (bypass) | 25 | Unused here; whole-pedal bypass, >= 64 engaged, < 64 bypassed |

CC numbers are drawn from MIDI 1.0's undefined controller range so they won't collide with mod wheel, volume, pan, expression, sustain, etc. Channel is omni, so CC/PC on any channel is accepted. Anything outside the map above, Program Change included, is passed to the callback registered with `hw.RegisterMidiEventCallback()`, so an effect can still define CCs of its own. Program Change is also latched and readable via `hw.GetProgramNumber()`, though this example doesn't act on it.

Footswitch CC state is available via `hw.GetFootswitchPressed()` (held) and `hw.GetFootswitchRisingEdge()` (momentary), and also feeds the normal/double/long-press callbacks from `hw.RegisterFootswitchCallbacks()`. All of them model a stomp, so a CC has to drop below 64 before it can assert again; set your controller to momentary, not latching, or a long press will never complete. Effects should read the footswitches through these rather than `hw.switches[FOOTSWITCH_n]`, which is physical-only and silently ignores CC 23/24. This example uses none of them, since a monophonic synth has no bypass or path-select concept.

Whole-pedal bypass is CC 25, separate from the footswitch CCs and absolute rather than momentary: >= 64 engages, < 64 bypasses, and repeating a value is a no-op. `Hothouse` owns that state (`GetBypass()` / `SetBypass()` / `ToggleBypass()`) so MIDI and the footswitch can't disagree about it; see `src/LibreVerb` for the pattern. This example has no bypass concept either, so CC 25 is received and stored but never read. Note that neither feeds `CheckResetToBootloader()`'s DFU-reset gesture: MIDI shouldn't be able to trigger a firmware reset.

### Controls

| CONTROL | DESCRIPTION | NOTES |
| - | - | - |
| KNOB 1 | FILTER | Moog ladder filter from 20Hz to 20kHz. Also CC 14. |
| KNOB 2 | RESONANCE | Watch out for self-oscillations and remember low frequencies with high resonance tend to boost a lot of energy (which could cause nasty clipping)! Also CC 15. |
| KNOB 3 | ATTACK | 0.001 sec to 0.25 sec. Also CC 16. |
| KNOB 4 | DECAY | 0.05 sec to 2 secs. Also CC 17. |
| KNOB 5 | Unused | |
| KNOB 6 | Unused | |
| SWITCH 1 | WAVEFORM | **UP** - SIN<br/>**MIDDLE** - POLYBLEP_SQUARE<br/>**DOWN** - POLYBLEP_SAW :metal: Also CC 20. |
| SWITCH 2 | Unused | CC 21 received, not acted on. |
| SWITCH 3 | Unused | CC 22 received, not acted on. |
| FOOTSWITCH 1 | DFU RESET | Hold *both* footswitches for 2 s to put the Daisy Seed in "flashable mode". CC 23 received, not acted on. |
| FOOTSWITCH 2 | Unused | CC 24 received, not acted on. |

### USB Host

It's assumed you'll be using a computer to flash this example, so you already have the hardware for a USB host. You just need to wire your MIDI controller to the Daisy Seed in software to send note and CC information. Most DAWs offer this functionality, but there are also lighter, FREE solutions like [Bome Network](https://www.bome.com/products/bomenet#downloads). I use Linux, so connecting my Launchkey Mini to the Daisy Seed via ALSA is a snap (Google ["aconnect alsa"](https://www.google.com/search?q=aconnect+alsa) if you don't know what I mean):

```console
newkular@linux-dev:~$ aconnect -i
client 0: 'System' [type=kernel]
    0 'Timer           '
    1 'Announce        '
client 14: 'Midi Through' [type=kernel]
    0 'Midi Through Port-0'
client 24: 'Launchkey Mini MK3' [type=kernel,card=2]
    0 'Launchkey Mini MK3 Launchkey Mi'
    1 'Launchkey Mini MK3 Launchkey Mi'
client 28: 'Daisy Seed Built In' [type=kernel,card=3]
    0 'Daisy Seed Built In MIDI 1'
newkular@linux-dev:~$ aconnect 24:0 28:0
```

![image](https://github.com/user-attachments/assets/b7f176ae-36c5-4c80-88aa-8601bdd60fe0)  
*Or you can use the QjackCtl Graph*
