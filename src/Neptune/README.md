# Neptune

Contributed by Ricky Sheaves \<<code@clevelandmusicco.com>\>

Ported from [GuitarML's Neptune](https://github.com/GuitarML/FunBox/tree/main/software/Neptune) for the Funbox, by Keith Bloemer. This is an awesome-sounding effect and super-popular in the DIY DSP community for good reason.

**FULL DISCLOSURE:** This was ported with Claude Code + human intervention. Otherwise, mistakes would have been made.

## Description

A stereo reverb and delay. The reverb is [CloudSeed](https://github.com/ValdemarOrn/CloudSeed), cut down to two delay lines per side so it fits on the Daisy Seed, with three of its factory programs on SWITCH 1. The delay runs up to 4 seconds in normal, octave-up or reverse mode, and SWITCH 2 decides whether it feeds the reverb or runs beside it.

Holding FOOTSWITCH 1 switches KNOBs 1, 2 and 4 to their alternate functions (listed in the NOTES column below). Let go and the knobs go back to their normal jobs, picking up wherever they're physically sitting.

### Controls

| CONTROL | DESCRIPTION | NOTES |
| - | - | - |
| KNOB 1 | Reverb decay | **Alternate:** reverb pre-delay |
| KNOB 2 | Dry/wet mix | **Alternate:** delay/reverb balance (CCW = all reverb, CW = all delay) |
| KNOB 3 | Delay time | 50 ms to 4 s. Below 1% the delay stops taking new input. |
| KNOB 4 | Reverb modulation depth | **Alternate:** reverb modulation rate |
| KNOB 5 | Reverb low-pass cutoff | Cubic taper |
| KNOB 6 | Delay feedback | |
| SWITCH 1 | Reverb program | **UP** - Chorus Delay<br/>**MIDDLE** - Medium Space<br/>**DOWN** - Rubi-Ka Fields |
| SWITCH 2 | Routing | **UP** - Delay plus delay into reverb<br/>**MIDDLE** - Delay and reverb in parallel<br/>**DOWN** - Delay into reverb (series) |
| SWITCH 3 | Delay mode | **UP** - Normal<br/>**MIDDLE** - Octave up<br/>**DOWN** - Reverse |
| FOOTSWITCH 1 | Alternate functions (hold) | LED 1 lights while held |
| FOOTSWITCH 2 | Bypass, with trails | LED 2 lights when engaged |

Hold both footswitches for 2 seconds to enter DFU (flashable) mode.

### Mono or stereo input

The default build reads only the left input and feeds it to both sides of the reverb and delay (mono in, stereo out). For true stereo in, build with:

```bash
make clean && make NEPTUNE_TRUE_STEREO=1
```

The `make clean` matters: changing the flag doesn't trigger a rebuild on its own.

### Bypass (trails)

Bypassing cuts the input to the delay and reverb over about 5 ms and leaves both running, so tails ring out instead of stopping dead. The dry signal passes at unity while bypassed, with the tails on top at whatever wet level KNOB 2 is set to.

### Changing reverb programs

CloudSeed has to clear its buffers to load a new program, so flipping SWITCH 1 mutes the wet signal for about 20 ms and the current reverb tail is lost. Flipping it back and forth quickly is rate-limited to one reload per mute.

### Known quirks

- Leaving alternate mode re-applies every knob's current position, so if you turned KNOB 1 for pre-delay, the decay jumps to wherever KNOB 1 now sits. Same for KNOBs 2 and 4. Alternate settings (pre-delay, mod rate, delay/reverb balance) survive until the next change to them.

## Building

Neptune fits in the Daisy Seed's internal flash (about 93% full), so it flashes like any other example; no bootloader needed.

```bash
make
make program-dfu
```

## Credits and licenses

This port is GPLv3, like the rest of this repository. It includes MIT-licensed code, with each original notice kept:

- **Neptune** by Keith Bloemer ([GuitarML](https://github.com/GuitarML/FunBox)). See `LICENSE-GuitarML`.
- **CloudSeed** reverb by Valdemar Erlingsson ([CloudSeed](https://github.com/ValdemarOrn/CloudSeed)), Daisy port by Erwin Coumans, further Daisy changes by GuitarML. See `cloudseed/license.txt`.
- **Reverse delay** from Adam Fulford's [Veno-Echo](https://github.com/AdamFulford/Veno-Echo), with changes by GuitarML. See `delayline_reverse.h`.
- **Octave delay** derived from DaisySP's `DelayLine` (Electrosmith), with changes by GuitarML. See `delayline_oct.h`.
- **Mix crossfade** from the [Signalsmith Audio blog](https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/).
