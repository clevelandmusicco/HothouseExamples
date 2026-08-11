# EchoKing MkII

Contributed by Cleveland Music Co. <code@clevelandmusicco.com>

A tape-echo effect modelling the Maestro Echoplex family (EP-1, EP-2, EP-3).
Three model profiles drive a shared engine -- tube and solid-state preamps,
per-pass feedback EQ, wow/flutter transport, SOS mode, and a Preamp-Only mode. A
second toggle ages the tape from new to worn, composing with the model so you
can hear EP-1-new through EP-3-worn and every combination in between. Delay time
is a single log sweep from 50 ms to 800 ms, so it's got a tiny bit more range
than the original hardware.

This is a reimagined version of the first EchoKing already in this repo. I
forked the original code shortly after Linus Torvalds adapted it for his [*very
cool* GuitarPedal project](https://github.com/torvalds/GuitarPedal). He
initially converted the effect to C and, in the process, ended up with spartan,
elegant code that was pleasing to look at, but didn't sound very good. So, we
tweaked and tuned to [make it sound
better](https://github.com/torvalds/GuitarPedal/commit/82313115779be3670f768a0e5ea87589c8ac8b39),
going down a rabbit hole trying to efficiently simulate the saturation curves of
vacuum tubes and old tape. (We [settled on a [5/4]-Padé approximant of
`tanhf`](https://github.com/torvalds/GuitarPedal/commit/505198a832184aec71d5d111518127c8374129f6),
if you're interested.)

That experience inspired me to revisit the EchoKing and make two major
algorithmic changes: proper component-level modelling of the tube and
solid-state transistor preamp circuits, and overhaul the tape write-path
saturation to model variable-knee static curve, level-dependent HF rolloff (bias
erasure), and envelope-gated hysteresis bias (probably better-stated as "tape-y
stuff"). I also simplified the UI a wee bit and added the **TAPE AGE** switch
for more creative uses. While I didn't use the [5/4]-Padé approximant Linus and
I settled on (I reverted back to a bias-tuned `tanhf` approach here), I *did*
apply a new, nearly-obsessive need to accurately model ageing, unreliable and
unpredictable analog circuits in DSP.

This is not a licensed product of or affiliated with Gibson Brands, Maestro, or
any other trademark holder. It is an educational DSP example.

## Controls

| Control        | Function         | Details                                                                          |
| ---            | ---              | ---                                                                              |
| KNOB 1         | **BLEND**        | equal-power dry/wet crossfade. Noon = equal levels.                              |
| KNOB 2         | **SUSTAIN**      | feedback/regeneration. Past 3 o'clock, self-oscillation begins.                  |
| KNOB 3         | **ECHO TIME**    | delay time, log taper from 50 ms (fully CCW) to 800 ms (fully CW).               |
| KNOB 4         | **RECORD LEVEL** | input drive into the tape path. Hotter = more saturation.                        |
| KNOB 5         | **TONE**         | playback EQ. CCW darkens the repeats; CW brightens them.                         |
| KNOB 6         | **WOW/FLUTTER**  | tape transport instability. CCW = stable; CW = seasick.                          |
| TOGGLESWITCH 1 | **MODEL**        | UP = EP-3 (solid state), MIDDLE = EP-2 (tube), DOWN = EP-1 (tube, oldest)        |
| TOGGLESWITCH 2 | **TAPE AGE**     | UP = new, MIDDLE = stock, DOWN = heavily-worn                                    |
| TOGGLESWITCH 3 | **MODE**         | UP = SOS (sound-on-sound), MIDDLE = normal echo, DOWN = preamp only              |
| FOOTSWITCH 1   | *unused*         | Hold *both* footswitches for 2 s to enter DFU (flashable) mode.                  |
| FOOTSWITCH 2   |                  | Toggles effect engage / bypass (trails: repeats ring out after bypass).          |
| LED 1          |                  | Flashes when SOS active; solid in Preamp-Only mode. Off by default.              |
| LED 2          |                  | Solid when effect is engaged.                                                    |

## Suggested settings

- **Classic slap (rockabilly, country):** EP-3, TAPE AGE new or stock, ECHO TIME
  fully CCW (~50 ms), SUSTAIN low (1-2 repeats), BLEND at noon.
- **Warm vintage echo (EP-2 at its best):** EP-2, TAPE AGE stock, ECHO TIME ~2
  o'clock (~350 ms), SUSTAIN ~noon, BLEND ~10 o'clock, RECORD LEVEL ~noon for
  mild saturation.
- **Preamp boost (no echo):** Any model, Preamp-Only mode. EP-3 = crisp focused
  boost; EP-2 = thicker, warmer color. Output runs a touch louder than
  tape-engaged mode (especially EP-3) -- the per-model post-trim is skipped here
  so the famous EP-3 boost is preserved.
- **Self-oscillation / dub delay:** EP-2 or EP-3, SUSTAIN past 3 o'clock. EP-3
  runs away more aggressively; TAPE AGE worn dirties the loop and destabilises
  pitch. Sweep ECHO TIME while oscillating.
- **Sound-on-Sound looping:** EP-2 or EP-3, SOS mode, ECHO TIME above noon,
  RECORD LEVEL moderate. TAPE AGE stock for clean accumulation; worn degrades
  each pass (musical for ambient, useless for tight rhythm).
- **Old, broken EP-1:** EP-1, TAPE AGE worn, WOW/FLUTTER cranked, TONE dark,
  SUSTAIN high. Should feel genuinely unstable.

## Operation

**SOS mode.** Clamps feedback at 0.985 (simulating a bypassed erase head, so the
loop barely decays). SUSTAIN is effectively bypassed; RECORD LEVEL controls how
aggressively new signal layers over the accumulating loop. Available on EP-2 and
EP-3 only -- on EP-1, toggleswitch 3 UP silently falls back to normal echo (EP-1
units predated Sound-on-sound mode).

**Preamp-Only mode.** Output is preamp-coloured dry signal; BLEND and the wet
path are bypassed. Runs slightly hotter than tape-engaged mode because the
per-model `preamp_makeup` post-trim is skipped to preserve EP-3's signature +3-5
dB boost. The tape loop still drains in the background (see Bypass below), so
switching back to echo doesn't dump a stale buffer at you.

**Bypass (trails).** Bypassing cuts the input to the tape loop but leaves the
loop running, so existing repeats decay away naturally instead of being frozen
mid-buffer. While bypassed the dry signal is clean and at unity -- the preamp is
out of the path -- and the trail rides on top at whatever wet level BLEND is
set to. Tape hiss is muted. Nothing new is recorded until the effect is
re-engaged.

The trail decays at the loop gain you've actually dialled in, which means it
does not always reach silence:

| Setting                   | Bypassed trail                                             |
| ---                       | ---                                                        |
| SUSTAIN below max         | Decays to silence; buffer clears in a few seconds.         |
| EP-2, SUSTAIN at max      | Loop gain 1.0 -- sustains indefinitely.                    |
| EP-3, SUSTAIN at max      | Loop gain 1.05 -- grows until the tape stage saturates.    |
| SOS mode                  | Loop gain 0.985 -- holds by design, decays over ~a minute. |

That's intentional: bypass is not a mute for the loop. If you want a guaranteed
clean slate, roll SUSTAIN back before or just after bypassing.

**WOW/FLUTTER.** Fully CCW disables mechanical modulation entirely. The knob
scales depth only; rates are fixed per model (EP-3 = least wobble, EP-1 = most).

**TAPE AGE.** Composes with model select rather than overriding it. EP-3-worn
still sounds like an EP-3 (solid-state preamp, brighter playback), just with a
tape path that's seen better days. The differences are subtle until RECORD LEVEL
and ECHO TIME are pushed up.

## DSP

**Tube preamp (EP-1, EP-2).** Two biased-`tanh` stages in series with a one-pole
DC-block between them (R = 0.995, ~38 Hz). Stage 1 uses 0.7x drive at the full
per-model asymmetry; stage 2 re-saturates the DC-cleaned signal at 0.9x drive
and 0.3x asymmetry so the offset doesn't stack. The interstage HPF models a
coupling cap. A second stage saturating an already-saturated signal generates
harmonic content -- including IM products -- a single waveshaper cannot, and
gives the preamp a slightly compressed feel under hard drive.

**EP-3 preamp.** Direct model of the single-stage common-emitter 2N5172 preamp
from the EP-3 schematic. Four blocks in series:

- Low-shelf cut (~120 Hz, ~6 dB) -- the 22 uF emitter-bypass cap parallel with
  the 2.2k emitter resistor.
- Asymmetric Class-A waveshaper: `tanhf(x*drive + 0.12) - tanhf(0.12)`. The
  off-centre bias produces 2nd-harmonic-dominant clipping that scales with input
  level.
- One-pole DC blocker (R = 0.997) for the drive-dependent residual offset.
- Miller-cap HF rolloff (~9 kHz) from C_cb * R_c.

Net ~+4 dB makeup captures the in-path boost the circuit is famous for. Reads as
"clean boost with forward mids."

**Tape stage.** Three layers run on the signal feeding the delay line, in this
order:

1. **Envelope-gated hysteresis bias.** Drive-scaled input is offset by
  `eff_hyst * env * hyst_state`, where `hyst_state` is a ~1 ms LPF of `sign(x[n]-x[n-1])`
   and `env` is a 3 ms / 30 ms rectified-signal follower. A proper M-H curve is
   a Jiles-Atherton ODE -- too expensive per sample on the M7. The LPF on
   `sign(dx)` kills the near-Nyquist flips that happen when the signal sits near
   zero; the envelope gate keeps the bias at zero during silence and decay tails
   so it doesn't smear into repeat fades.
2. **Bias-erasure LPF.** Cutoff is `record_fc * (1 - eff_erasure * env)`, so
   loud transients lose high end before the curve gets to them. `SetCutoff`
   (`expf` inside) runs once every 4 samples -- cheap and inaudible.
3. **Variable-knee static curve.** `y = tanhf(x_d / (1 + |x_d|^n)^(1/n))`, where
   `n` is per-model in {2, 3, 4} (n=2 = softest, oldest tape; n=4 = firmest
   shoulder). Integer `n` lets the hot loop use `sqrtf` / `cbrtf` / nested
   `sqrtf` instead of `powf`.

The output is divided by `eff_drive` so small-signal gain stays at 1.0 -- drive
shapes the knee without secretly multiplying feedback-loop gain. The outer
`tanhf` bounds the asymptote near +/-0.76 so high-SUSTAIN loops settle short of
the rail.

## Implementation patterns

- **Model-profile abstraction.** All per-model behaviour lives in `constexpr
  ModelProfile`. Switching models swaps a pointer; the hot loop has zero
  per-model branches.
- **Orthogonal deformation.** Tape age is a separate `constexpr TapeAge` of
  multipliers applied at the point of use. Any age * any model is valid; no `if
  model == EP1 && age == worn` anywhere.
- **Read before write.** The delay line is read before the new write sample is
  computed. The other order produces zero-latency feedback -- topologically
  wrong and sounds terrible.
- **Per-pass feedback EQ.** A dedicated one-pole LPF in the feedback path
  darkens the loop on every pass. This is the single biggest reason a tape delay
  doesn't sound like a digital delay with wet coloring.
- **Proportional wow/flutter depth.** Depths are `delay * 0.5%` (wow) and
  `delay * 0.04%` (flutter). The same fractional speed error produces more absolute
  pitch variation at longer delays -- matches the real machine.
- **Wandering wow (nested LFO).** A ~0.15 Hz secondary oscillator modulates the
  wow LFO rate by +/-25%. The wobble drifts instead of cycling steadily; this is
  what distinguishes wow from chorus / vibrato.
  