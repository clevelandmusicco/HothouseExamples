# EchoKing

Contributed by Cleveland Music Co. <code@clevelandmusicco.com>

A digital model of the Maestro Echoplex family of tape delay units. Three
models are selectable: the EP-1 (tube, early vintage), the EP-2 (tube, classic
warm), and the EP-3 (solid state, tight and focused). Each model captures the 
distinct character of its preamp, tape path bandwidth, repeat darkening behavior, 
and transport mechanics.

The signal always passes through the preamp, so even with BLEND fully dry the
model's coloration is present in the output. This is historically accurate: the
Echoplex preamp is always in circuit, and players have long used Echoplex units
as standalone tone shapers (especially the EP-3's well-regarded preamp boost).
Preamp-Only mode (TOGGLESWITCH 3 down) exploits this directly.

This is not a licensed product of or affiliated with Gibson Brands, Maestro, or
any other trademark holder. It is an educational DSP example.

## Controls

| Control        | Function                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------ |
| KNOB 1         | **BLEND** -- equal-power dry/wet crossfade. Noon = equal levels.                          |
| KNOB 2         | **SUSTAIN** -- feedback/regeneration. Past 3 o'clock, self-oscillation begins.             |
| KNOB 3         | **ECHO TIME** -- delay time, log taper within the range set by TOGGLESWITCH 2.            |
| KNOB 4         | **RECORD LEVEL** -- input drive into the tape path. Hotter = more saturation.             |
| KNOB 5         | **TONE** -- playback EQ. CCW darkens the repeats; CW brightens them.                      |
| KNOB 6         | **WOW/FLUTTER** -- tape transport instability. CCW = stable; CW = seasick.                |
| TOGGLESWITCH 1 | **MODEL**: UP = EP-3 (solid state), MIDDLE = EP-2 (tube, classic), DOWN = EP-1 (tube, vintage) |
| TOGGLESWITCH 2 | **ECHO RANGE**: UP = short (50-400 ms), MIDDLE = medium (100-600 ms), DOWN = long (200-800 ms) |
| TOGGLESWITCH 3 | **MODE**: UP = SOS (sound-on-sound), MIDDLE = normal echo, DOWN = preamp only             |
| FOOTSWITCH 2   | Engage / bypass.                                                                           |
| LED 1          | Lit when SOS or Preamp-Only mode is active.                                                |
| LED 2          | Lit when effect is engaged.                                                                |

**Note on SOS mode:** Sound-on-Sound locks feedback at 1.0 (simulating a bypassed
erase head so the loop never decays). SUSTAIN is effectively bypassed; RECORD LEVEL
controls how aggressively new signal layers over the accumulating loop. SOS is
available on EP-2 and EP-3 only. On EP-1, TOGGLESWITCH 3 UP silently falls back
to normal echo (EP-1 units predated explicit SOS as a product feature). Pedantic?
Oh, it's hella pedantic 🤓

**Note on ECHO RANGE:** On the real Echoplex, delay range was set by repositioning
the recording head relative to the playback head -- a coarse mechanical adjustment.
TOGGLESWITCH 2 models that selector; KNOB 3 then fine-tunes within the chosen range.

## Suggested settings

- **Classic slap (rockabilly, country):** EP-3, short range, ECHO TIME around 9 o'clock,
  SUSTAIN low (1-2 repeats), BLEND at noon.
- **Warm vintage echo (EP-2 at its best):** EP-2, medium range, ECHO TIME around noon,
  SUSTAIN at about noon, BLEND slightly wet. RECORD LEVEL around noon for mild saturation.
- **Preamp boost (no echo):** Any model, Preamp-Only mode. RECORD LEVEL and BLEND
  no longer affect echo; the output is just the preamp-colored dry signal. EP-3 gives
  a crisp focused boost; EP-2 gives a thicker, warmer color.
- **Self-oscillation / dub delay:** EP-2 or EP-3, SUSTAIN past 3 o'clock. With EP-3
  the runaway is a bit more aggressive. Sweep ECHO TIME while oscillating for pitch effects.
- **Sound-on-Sound looping:** EP-2 or EP-3, SOS mode, medium or long range. Set RECORD
  LEVEL moderate and play phrases. Each phrase layers on top of the last. Hit FOOTSWITCH 2
  to bypass and hear your loop dry, or sweep BLEND to taste.
- **Old, broken EP-1:** EP-1, WOW/FLUTTER cranked, TONE dark, SUSTAIN high. Should feel
  genuinely unstable.

## Notes for DSP learners

The source file `echo_king.cpp` demonstrates several useful techniques:

- **Model-profile abstraction:** All per-model behavior (EQ, saturation, wow depth, etc.)
  lives in `constexpr ModelProfile` structs. The audio engine has no per-model branches
  in its hot loop -- switching models just changes which profile pointer is active.
  This is a clean pattern for multi-model effects.

- **Read before write in feedback delays:** The delay line is read before the new write
  value is computed. Reading after writing would produce zero-latency feedback (the
  just-written sample immediately feeds back), which sounds broken and is topologically
  wrong. Read, compute, write -- always.

- **Per-pass feedback EQ:** A dedicated one-pole LPF in the feedback path cuts high
  frequencies on every pass around the loop. This is the primary reason tape delay
  repeats sound the way they do: each echo is darker than the last. Without this,
  a tape delay model is just a digital delay with extra coloring on the wet output.

- **Proportional wow/flutter depth:** Wow and flutter depths are computed as a
  percentage of the current delay time (`delay * 0.5%` for wow). This matches the
  physics of a real transport: the same fractional speed error produces proportionally
  more absolute pitch variation at longer delay times.

- **Asymmetric tube waveshaper:** `tanhf(x * drive + bias) - tanhf(bias)` shifts the
  tanhf operating point off center. Different gains on positive vs. negative half-cycles
  generate even-order harmonics (2nd, 4th...) -- the source of "tube warmth." Subtracting
  `tanhf(bias)` removes the output DC offset that the bias would otherwise introduce.

- **tanhf for feedback safety:** The write path passes through `tanhf()` before entering
  the delay buffer. This bounds all values in the buffer to (-1, 1) regardless of how
  high feedback is pushed. Self-oscillation converges to a stable saturated amplitude
  instead of an unbounded numeric blowup. The softness of the clamp also sounds
  better than a hard digital ceiling.

- **Wandering wow (nested LFO):** A very slow (~0.15 Hz) secondary oscillator modulates
  the wow LFO's rate by +/-25%. This makes the wow period drift rather than cycle
  at a perfectly steady interval, more closely resembling real motor irregularity.
