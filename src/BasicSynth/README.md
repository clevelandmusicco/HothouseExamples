# BasicSynth

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

Super basic monophonic synthesizer example with multiple waveforms, Moog-style filter, resonance, attack and decay. Note information is read via MIDI over USB, so you will need a USB host (your computer, a [commercial](https://www.google.com/search?q=USB+midi+host) or [DIY solution](https://youtu.be/N4yUduOqR3M?feature=shared)). Just flash the example and the Hothouse becomes a simple synth.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | FILTER | Moog ladder filter from 20Hz to 20kHz |
| KNOB 2 | RESONANCE | Watch out for self-oscillations! |
| KNOB 3 | ATTACK | 0.001 sec to 0.25 sec |
| KNOB 4 | DECAY | 0.05 sec to 2 secs |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | Unused | **UP** - SIN<br/>**MIDDLE** - POLYBLEP_SQUARE<br/>**DOWN** - POLYBLEP_SAW :metal: |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | RESET | Hold down for 2 seconds to put the Daisy Seed in "flashable mode" |
| FOOTSWITCH 2 | Unused |  |

### USB Host

It's assumed you'll be using a computer to flash this example, so you already have the hardware for a USB host. You just need to wire your MIDI controller to the Daisy Seed in software to send note information. Most DAWs offer this functionality, but there are also lighter, FREE solutions like [Bome Network](https://www.bome.com/products/bomenet#downloads). I use Linux, so connecting my Launchkey Mini to the Daisy Seed via ALSA is a snap (Google ["aconnect alsa"](https://www.google.com/search?q=aconnect+alsa) if you don't know what I mean):

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
