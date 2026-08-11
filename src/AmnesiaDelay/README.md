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
| FOOTSWITCH 1  | *(unused)* Hold *both* footswitches for 2 s to enter DFU (flashable) mode.                |
| FOOTSWITCH 2  | Engage / bypass, with trails (see below).                                                 |

## Bypass (trails)

Bypassing cuts the input to the delay line but leaves the loop running, so
existing repeats decay away instead of being frozen mid-buffer and dumped back
at you on re-engage. The dry signal passes clean at unity while bypassed and the
trail rides on top at whatever wet level **BLEND** is set to. Clock noise and
hiss are muted, since both feed the loop and would otherwise keep it topped up
forever. Nothing new is recorded until the effect is re-engaged.

The trail decays at the loop gain you've dialled in, so it doesn't always reach
silence: **FEEDBACK** runs to 1.05, and past unity a bypassed trail keeps
self-oscillating (bounded by the soft clipper) rather than dying out. Roll
**FEEDBACK** back if you want a guaranteed clean slate.

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
