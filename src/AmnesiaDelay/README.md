# Amnesia BBD-Style Delay

Contributed by Cleveland Music Co. <code@clevelandmusicco.com>

A BBD-style delay loosely modeled on the Electro-Harmonix Deluxe Memory Man. This is not a component-level model and is not affiliated with or endorsed by Electro-Harmonix; it is an educational example meant to capture the broad behavior: darkening repeats, delay-time pitch swoops, musical runaway feedback, chorus/vibrato modulation applied to the delayed signal, and a simulated BBD clock-noise effect.

## Controls

| Control       | Function                                                                                  |
| ------------- | ----------------------------------------------------------------------------------------- |
| KNOB 1        | **BLEND** -- equal-power crossfade between dry and wet. Noon = equal levels.               |
| KNOB 2        | **FEEDBACK** -- number of repeats. Maxed out, runs into (gentle) self-oscillation.         |
| KNOB 3        | **DELAY** -- delay time, ~30 ms (CCW) to ~550 ms (CW), log taper.                          |
| KNOB 4        | **DEPTH** -- modulation depth applied to the delay-time read tap.                          |
| KNOB 5        | **RATE** -- modulation rate. CCW = chorus (slow); CW = vibrato (fast).                     |
| KNOB 6        | **CLOCK NOISE** -- BBD clock leakage and bias-drift hiss. CCW = clean; CW = wonky DMM.     |
| FOOTSWITCH 2  | Engage / bypass.                                                                          |

## Some recommended settings

* For a rich, vintage chorus tone, set **DELAY** fairly short, **BLEND** at noon, **DEPTH** high, **RATE** low (chorus side), and **FEEDBACK** fairly high.
* For a "Doppler" vibrato, push **RATE** clockwise and turn **BLEND** fully wet.
* For runaway oscillation, push **FEEDBACK** to the right end of its travel.
* For pitch-bent eerie effects, sweep the **DELAY** knob while playing, especially with high **FEEDBACK**.
* **CLOCK NOISE** is meant to simulate an old, drifted DMM. It's subtle at lower **BLEND** and shorter **DELAY** settings; crank both of those knobs to really hear the hiss and whine. Silly? Sure. But that 6th knob was just gagging for something to do 🫢

## Notes for DSP learners

The source file `amnesia_delay.cpp` is heavily commented and demonstrates a handful of useful DSP idioms:

* **SDRAM-backed DelayLine** with the `DSY_SDRAM_BSS` placement attribute.
* **Equal-power crossfade** using cosine/sine instead of a linear interpolation (lerp), so the perceived volume stays constant across the BLEND sweep.
* **Phase-accumulator LFO** that feeds `sinf()` directly every sample; no wavetable, no lookup quantization, no zero-reset dead zone.
* **Cascaded one-pole low-passes** approximating a steep BBD anti-alias filter whose cutoff tracks the delay time (longer delay = darker repeats).
* **Aliased clock-tone synthesis**: the BBD clock frequency is computed from the delay length and synthesised without band-limiting, so the audible fold-back at low delay times sounds correct.
* **`fonepole` parameter smoothing** to eliminate zipper noise from knob moves.
* **`tanhf` soft clipping** in the feedback loop to keep self-oscillation musical and bounded without changing the runaway character.
