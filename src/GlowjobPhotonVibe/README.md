# Glowjob Photon Vibe

A reasonably authentic model of the Shin-ei Uni-Vibe (1968) - the photocell
phaser/chorus pedal made famous by Jimi Hendrix and Robin Trower. This was
an educational exercise, but I like the results.

## What makes it different from a standard phaser

Most phasers sweep four all-pass stages tuned to the *same* frequency. The
Uni-Vibe uses four stages with *different* capacitors (0.015, 0.022, 0.047,
and 0.1 µF from the original Shin-ei schematic). A single incandescent lamp
and four LDRs (light-dependent resistors) modulate all stages together, but
the unequal caps cluster the notches densely at low frequencies - that's the
"thick, wobbly" character nobody has been able to stop chasing since 1968.

The other secret: LDRs respond asymmetrically to light changes. Resistance
drops relatively quickly as the lamp brightens but rises slowly as it dims.
This skews the sine LFO into a lopsided waveform unique to the lamp-and-LDR
circuit. SWITCH 3 and KNOB 3 let you tune how much of that asymmetry you hear.

## Controls

| Control | Function |
| --- | --- |
| KNOB 1 | SPEED - LFO rate from ~0.5 Hz (slow throb) to 10 Hz (fast shimmer), log taper |
| KNOB 2 | INTENSITY - LFO depth; lower values park the sweep toward its midpoint |
| KNOB 3 | LAG - photocell response time; CCW = snappy, CW = more laggy and dreamy |
| KNOB 4 | MIX - wet/dry blend in Chorus mode (ignored in Vibrato mode) |
| KNOB 5 | VOLUME - output level (up to +3 dB to compensate for Vibrato mode's missing dry signal) |
| KNOB 6 | FEEDBACK - notch resonance depth when SWITCH 2 is UP/MIDDLE; adds some "whomp" |
| SWITCH 1 | MODE - UP: Chorus (dry+wet mix), DOWN: Vibrato (wet signal only, pure pitch mod) |
| SWITCH 2 | FEEDBACK - UP/MIDDLE: Feedback active (KNOB 6 controls depth), DOWN: Feedback off |
| SWITCH 3 | LDR MODEL - UP: Modern (subtle asymmetry, 3:1 ratio), MIDDLE: Vintage Shin-ei (classic wobble, 6:1), DOWN: Sluggish (exaggerated, 12:1) |
| FOOTSWITCH 1 | (Hold 2 seconds to enter DFU/bootloader mode) |
| FOOTSWITCH 2 | Bypass toggle |
| LED 1 | Lit when Vibrato mode is selected |
| LED 2 | Lit when effect is active (not bypassed) |

## Tips

- Classic Hendrix tone: SWITCH 1 to Chorus, SPEED around 10-11 o'clock, INTENSITY
  at noon, SWITCH 3 to Vintage (MIDDLE), LAG at noon, MIX at noon.
- Robin Trower vibrato: SWITCH 1 to Vibrato, slower Speed, higher Intensity.
- The Feedback + Sluggish LDR combination gets you into wah-adjacent territory
  at higher Feedback levels - especially at slower speeds.

## TODO

- Remap the controls to a more intuitive UX; some switches and knobs would probably
make more sense rearranged and/or adjacent to one another.
