# LibreVerb

Contributed by Ricky Sheaves \<<ricky@clevelandmusicco.com>\>

## Description

A true-stereo reverb based on [Freeverb](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html) (Jezar at Dreampoint), adapted for the Hothouse and pushed quite a bit past the original feature set.

The classic Freeverb structure is here: 8 parallel lowpass-feedback comb filters and 4 series Schroeder allpasses per channel, with left/right delay-length asymmetry providing decorrelated stereo from a mono input. In addition:

- **In-loop highs and lows damping.** The HIGHS knob's lowpass and the LOWS knob's highpass both sit inside the comb feedback path, so they shape how the tail's spectrum evolves over time (a final tone control would just filter the output, which is not the same thing).
- **Modulated comb taps** with one LFO per channel and a fixed per-comb phase offset, for a chorused Lexicon-style tail.
- **Plate / Hall / Cathedral** size modes: the same algorithm with the delay buffers scaled by 0.6x, 1.0x, and 1.4x respectively. The size-glide between modes sounds like the audio version of a "dolly zoom".
- **Mono / Stereo / Wide** stereo-width modes. The wet bus is recombined as mid/side (`M = 0.5*(L+R)`, `S = 0.5*(L-R)`): regular stereo is just the plain L/R pair, mono folds the two decorrelated tails into one amplitude-matched channel, and wide pushes the side content up and trims the mid a touch for a noticeably bigger field. The width transition is smoothed so flicking the switch doesn't click.
- **DC blocker** on the input and a small safety floor on the LOWS HPF so very long decay times don't ring up sub-bass mud.

The mono dry signal is split into two banks of combs with slightly different delay lengths (the "stereo spread"), so the left and right tails decorrelate naturally instead of being a copy of one channel.

### MIDI CC mapping

Every knob, every toggleswitch, and both footswitches can be driven by MIDI CC over USB, so you'll need a USB host (your computer, a [commercial](https://www.google.com/search?q=USB+midi+host) or [DIY solution](https://youtu.be/N4yUduOqR3M?feature=shared)).

Knobs and toggles use hard takeover: a CC overrides the physical control until you move the pot (or flip the switch), at which point the physical control reclaims it. There's no pickup/soft-takeover, so the value can jump on reclaim. Footswitches are a plain OR of physical and CC state, nothing to reclaim.

| CONTROL | CC # | PARAM |
| - | - | - |
| KNOB 1 | 14 | MIX |
| KNOB 2 | 15 | DECAY |
| KNOB 3 | 16 | PRE-DELAY |
| KNOB 4 | 17 | HIGHS |
| KNOB 5 | 18 | LOWS |
| KNOB 6 | 19 | MODULATION |
| SWITCH 1 | 20 | WIDTH (CC 0-42 Mono, 43-85 Stereo, 86-127 Wide) |
| SWITCH 2 | 21 | SIZE (CC 0-42 Plate, 43-85 Hall, 86-127 Cathedral) |
| SWITCH 3 | 22 | MOD SPEED (CC 0-42 Slow, 43-85 Moderate, 86-127 Fast) |
| FOOTSWITCH 1 | 23 | Unused (received, not acted on) |
| FOOTSWITCH 2 | 24 | Press of footswitch 2, i.e. toggles bypass |
| (bypass) | 25 | Bypass: >= 64 engages the reverb, < 64 bypasses it |

CC numbers come from MIDI 1.0's undefined controller range, so no collision with mod wheel, volume, pan, expression, sustain, etc. Channel is omni. The DFU-reset gesture is deliberately not reachable over MIDI: CC 23/24 can't trigger a firmware reset.

CC 23/24 mean "press that footswitch", whatever it happens to be wired to. Here footswitch 2 is bypass, so CC 24 toggles bypass; in an effect where footswitch 2 is a freeze or a tap, CC 24 would do that instead. They model *momentary presses*: a press needs a matching release before it can press again.

Bypass also gets its own CC, which is what most MIDI-capable pedals do. It's a state the host should be able to set outright, not toggle: send 127 and the reverb is engaged whether or not it already was, send 0 and it's bypassed. Repeating the same value does nothing. That's how the host stays in sync after a preset change or a reconnect, which a toggle-only mapping can't do. Values are thresholded at 64 rather than matched against 0/127 exactly, same convention as a sustain pedal, so a controller sending 100 still works. Stomping footswitch 2 toggles as always; last one to move wins.

### Controls

| CONTROL | DESCRIPTION | NOTES |
| - | - | - |
| KNOB 1 | MIX | Equal-power dry/wet crossfade. ~3 o'clock = equal dry+wet. Also CC 14. |
| KNOB 2 | DECAY | Reverb time. Maps comb feedback from ~0.7 (about 0.5s) up to 0.999 (about 30s RT60 on the longest comb in Hall mode). Also CC 15. |
| KNOB 3 | PRE-DELAY | 1 sample to ~250 ms. Logarithmic taper -- most of the dial is spent on the short end. Also CC 16. |
| KNOB 4 | HIGHS | Damping amount for the highs in the tail. CCW = LPF@24kHz, CW = LPF@~400Hz. Lives in the comb feedback path, so it shapes the spectrum over successive bounces. Also CC 17. |
| KNOB 5 | LOWS | Damping amount for the lows in the tail. CCW = HPF@~20Hz, CW = HPF@~400Hz. Also in the comb feedback path; a small floor is kept on always for stability at long decays. Also CC 18. |
| KNOB 6 | MODULATION | LFO depth (0 = static reverb, max = ~16 samples peak-to-peak per comb tap). Also CC 19. |
| SWITCH 1 | WIDTH | **UP** - Mono (both reverb tails folded to one channel, mono-compatible) <br/>**MIDDLE** - Stereo (the regular stereo tail) <br/>**DOWN** - Wide (mid/side widened, bigger stereo image). Also CC 20. |
| SWITCH 2 | SIZE | **UP** - Plate (tight, bright, short tail) <br/>**MIDDLE** - Hall (_almost_ stock Freeverb) <br/>**DOWN** - Cathedral (bigger, slower, sparser). Glides between modes when flicked. Also CC 21. |
| SWITCH 3 | MOD SPEED | **UP** - Slow (0.15 Hz, classic Lexicon) <br/>**MIDDLE** - Moderate (0.5 Hz, Blue Sky territory) <br/>**DOWN** - Fast (1.6 Hz, warbly). Also CC 22. |
| FOOTSWITCH 1 | (unused) | Hold *both* footswitches for 2 s to enter DFU mode. CC 23 received, not acted on. |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered. LED 2 lit when the effect is engaged. CC 24 toggles it like a stomp; CC 25 sets it outright (>= 64 engaged, < 64 bypassed). |

### Tips and suggested settings combinations

- **Anti-gravity room**: Cathedral size + max MODULATION + slow MOD SPEED + Wide width = a room that drifts and breathes. With LOWS pulled back it sounds vast and weightless.
- **Plate slap**: Plate size + low DECAY + minimal MOD + Mono width = punchy short reverb that sits tight and centred behind a clean guitar.
- **Detuned wreck**: max MODULATION + fast MOD SPEED + long DECAY in Cathedral mode. Not subtle. Good for ambient.
- **Mono check**: flick WIDTH to UP to hear exactly what the reverb collapses to -- handy if the signal is going to end up summed somewhere downstream.
