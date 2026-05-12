# LibreVerb

Contributed by Ricky Sheaves \<<ricky@clevelandmusicco.com>\>

## Description

A true-stereo reverb based on [Freeverb](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html) (Jezar at Dreampoint), adapted for the Hothouse and pushed a fair way past the original feature set.

The classic Freeverb structure is here -- 8 parallel lowpass-feedback comb filters and 4 series Schroeder allpasses per channel, with left/right delay-length asymmetry providing decorrelated stereo from a mono input. On top of that:

- **In-loop highs and lows damping.** The HIGHS knob's lowpass and the LOWS knob's highpass both sit inside the comb feedback path, so they shape how the tail's spectrum evolves over time (a final tone control would just colour the output, which is not the same thing).
- **Modulated comb taps** with one LFO per channel and a fixed per-comb phase offset, for a chorused Lexicon-style tail.
- **Plate / Hall / Cathedral** size modes via Switch 2: the same algorithm with the delay buffers scaled by 0.6x, 1.0x, and 1.4x respectively. The size-glide between modes is intentional and very nice.
- **Mono / Stereo / Wide** stereo-width modes via Switch 1. The wet bus is recombined as mid/side (`M = 0.5*(L+R)`, `S = 0.5*(L-R)`): regular stereo is just the plain L/R pair, mono folds the two decorrelated tails into one channel (level-matched so it doesn't jump), and wide pushes the side content up and trims the mid a touch for a noticeably bigger field. The width transition is smoothed so flicking the switch doesn't click.
- **DC blocker** on the input and a small safety floor on the LOWS HPF so very long decay times don't ring up sub-bass mud.

The mono dry signal is split into two banks of combs with slightly different delay lengths (the "stereo spread"), so the left and right tails decorrelate naturally instead of being a copy of one channel.

### Controls

| CONTROL | DESCRIPTION | NOTES |
| - | - | - |
| KNOB 1 | MIX | Equal-power dry/wet crossfade. ~3 o'clock = equal dry+wet. |
| KNOB 2 | DECAY | Reverb time. Maps comb feedback from ~0.7 (about 0.5s) up to 0.999 (about 30s RT60 on the longest comb in Hall mode). |
| KNOB 3 | PRE-DELAY | 1 sample to ~250 ms. Logarithmic taper -- most of the dial is spent on the short end. |
| KNOB 4 | HIGHS | Damping amount for the highs in the tail. CCW = LPF@24kHz, CW = LPF@~400Hz. Lives in the comb feedback path, so it shapes the spectrum over successive bounces. |
| KNOB 5 | LOWS | Damping amount for the lows in the tail. CCW = HPF@~20Hz~, CW = HPF@~400Hz. Also in the comb feedback path; a small floor is kept on always for stability at long decays. |
| KNOB 6 | MODULATION | LFO depth (0 = static reverb, max = ~12 samples peak-to-peak per comb tap). |
| SWITCH 1 | WIDTH | **UP** - Mono (both reverb tails folded to one channel, mono-compatible) <br/>**MIDDLE** - Stereo (the regular stereo tail) <br/>**DOWN** - Wide (mid/side widened -- bigger stereo image). |
| SWITCH 2 | SIZE | **UP** - Plate (tight, bright, short tail) <br/>**MIDDLE** - Hall (the stock Freeverb feel) <br/>**DOWN** - Cathedral (bigger, slower, sparser). Glides between modes when flicked. |
| SWITCH 3 | MOD SPEED | **UP** - Slow (0.15 Hz, classic Lexicon) <br/>**MIDDLE** - Moderate (0.5 Hz, Blue Sky territory) <br/>**DOWN** - Fast (1.6 Hz, warbly). |
| FOOTSWITCH 1 | (unused) | Hold for 2s to enter DFU mode. |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered. LED 2 lit when the effect is engaged. |

### Tips and suggested settings combinations

- **Anti-gravity room**: Cathedral size + max MODULATION + slow MOD SPEED + Wide width = a room that drifts and breathes. With LOWS pulled back it sounds vast and weightless.
- **Plate slap**: Plate size + low DECAY + minimal MOD + Mono width = punchy short reverb that sits tight and centred behind a clean guitar.
- **Detuned wreck**: max MODULATION + fast MOD SPEED + long DECAY in Cathedral mode. Not subtle. Good for ambient.
- **Mono check**: flick WIDTH to UP to hear exactly what the reverb collapses to -- handy if the signal is going to end up summed somewhere downstream.
