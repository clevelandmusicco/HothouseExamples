# MIDI on the Hothouse

Effects that call `hw.StartMidi()` accept MIDI over USB. The Hothouse enumerates
as a class-compliant USB MIDI **device**, so it needs a USB host at the other end:
your computer, or a [commercial](https://www.google.com/search?q=USB+midi+host)
or [DIY](https://youtu.be/N4yUduOqR3M?feature=shared) USB host box.

Not every example uses MIDI. Check the effect's own README for its CC table.

## CC map

Every knob, toggleswitch, and footswitch is drivable by CC. The numbers are
fixed in `src/hothouse.cpp` and are the same in every effect that supports MIDI:

| CONTROL | CC # | VALUE |
|-|-|-|
| KNOB 1-6 | 14-19 | 0-127 across the knob's range |
| SWITCH 1-3 | 20-22 | 0-42 UP, 43-85 MIDDLE, 86-127 DOWN |
| FOOTSWITCH 1 | 23 | Momentary press, >= 64 is down |
| FOOTSWITCH 2 | 24 | Momentary press, >= 64 is down |
| (bypass) | 25 | >= 64 engaged, < 64 bypassed |

14-25 sit in MIDI 1.0's undefined controller range, so nothing here collides
with mod wheel, volume, pan, expression, or sustain.

CC 23/24 model a *press* of that footswitch, so they do whatever the footswitch
does in the effect you're running. Set your controller to momentary, not
latching, if the footswitch is a hold-to-do-something function. CC 25 sets
whole-pedal bypass outright regardless of what the footswitches are wired to,
which is how a host resyncs state after a preset change or a reconnect.

Knobs and toggles use hard takeover: a CC overrides the physical control until
you move the pot or flip the switch, at which point the physical control takes
it back. There's no soft pickup, so values can jump on reclaim.

Program Change is received and latched. Anything outside the map above, PC
included, is handed to the effect, which may or may not do something with it.

## MIDI channel

Every binary has a **factory channel**, fixed when it was compiled: omni unless
it was built with `-DHOTHOUSE_MIDI_CHANNEL=n` (see [compile-time
options](#compile-time-options)). Stock builds and everything on the releases
page are omni, meaning they respond to every channel.

Omni is fine for one pedal and a bad time on a board with two of them, so the
channel is also settable on the pedal itself, no recompiling and no computer
beyond whatever is already sending the MIDI. A channel set that way is saved and
overrides the factory channel until you reset it.

The gestures below are identical in every MIDI-capable effect, including the
pre-built binaries on the
[releases page](https://github.com/clevelandmusicco/HothouseExamples/releases).

### Setting the channel

1. Power the pedal up with **FOOTSWITCH 1 held down**.
2. Both LEDs blink together. That's "listening", and it lasts 10 seconds.
3. Send anything from your controller on the channel you want: a note, a CC, a
   program change. The first message that arrives wins.
4. The LEDs blink back the channel it picked (see decoding below) and the pedal
   boots normally. The channel is saved and survives power cycles and reflashes.

If nothing arrives within 10 seconds, both LEDs give one long flash, the channel
is left alone, and the pedal boots. Nothing is lost by entering this by accident.

One deliberate exception: "reset all controllers" and the other channel-mode
messages (CC 120-127) are ignored during learn, because plenty of hosts
broadcast those at startup and they'd pick the channel for you.

### Resetting to the factory channel

Power the pedal up with **FOOTSWITCH 2 held down**. The saved channel is
forgotten and the LEDs blink back the factory channel. On a stock build that's
omni: both LEDs double-blink together and the pedal responds on every channel
again.

Holding *both* footswitches at power-up does nothing to the MIDI channel. That
grip is the DFU gesture, and it shouldn't reconfigure MIDI on the way past.

### Reading the channel back

If the channel is anything other than omni, the pedal blinks it out at every
power-up before audio starts. **LED 1 flashes the tens digit, LED 2 flashes the
ones.**

| CHANNEL | LED 1 | LED 2 |
|-|-|-|
| omni | both LEDs double-blink together | |
| 1 | (none) | 1 flash |
| 7 | (none) | 7 flashes |
| 10 | 1 flash | (none) |
| 13 | 1 flash | 3 flashes |
| 16 | 1 flash | 6 flashes |

Omni boots silently, so if you don't use MIDI you'll never see any of this.
Channel 16 is the slowest case at roughly two seconds.

### What the channel applies to

Everything that carries a channel: the CC map above, effect-specific CCs, notes,
program change, pitch bend, aftertouch. Messages that carry no channel at all
(MIDI clock, transport, sysex) can't be filtered and always reach
`RegisterMidiEventCallback()`, whatever channel the pedal is on.

`Hothouse` does not consume clock itself and there are no plans for it to. An
effect that wants tempo sync counts `TimingClock` (24 per quarter note) in its
own MIDI callback, because how you smooth the tempo and what you do when clock
stops are effect-specific choices, not something the shared class should pick.

## Compile-time options

Only relevant if you build your own binaries. Both are `#ifndef`-guarded in
`src/hothouse.h`, so a `-D` in the effect's Makefile wins.

| DEFINE | DEFAULT | MEANING |
|-|-|-|
| `HOTHOUSE_MIDI_CHANNEL` | `0` | The binary's factory channel; 0 is omni, 1-16 is a channel. A channel set on the pedal overrides it until FOOTSWITCH 2 at power-up clears it, and changing this and reflashing only takes effect on a pedal that has no saved channel. |
| `HOTHOUSE_SETTINGS_QSPI_OFFSET` | `0x400000` | Where the settings block lives on the 8 MB QSPI chip. 4 MB in, well clear of a program flashed to QSPI by the Daisy bootloader. |

Set them with `C_DEFS` in the effect's Makefile, alongside the other project
variables and before the `include` lines. This is the same knob libDaisy's own
core Makefile uses, and it reaches the C++ sources as well as the C ones:

```make
# Project Name
TARGET = my_effect

# Sources and Hothouse header files
CPP_SOURCES = my_effect.cpp ../hothouse.cpp
C_INCLUDES = -I..

# Default to MIDI channel 4 instead of omni
C_DEFS += -DHOTHOUSE_MIDI_CHANNEL=4

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Global helpers
include ../Makefile
```

Run `make clean` after changing a define. The dependency files track headers,
not the Makefile, so an incremental build will happily relink stale objects that
still have the old value baked in.

From code, `hw.GetMidiChannel()` and `hw.SetMidiChannel()` read and write it.
`SetMidiChannel()` erases and writes flash, so call it before `StartAudio()` and
never from the audio callback.

## Gotchas

- `hw.StartMidi()` claims the internal USB peripheral, so it can't coexist with
  `seed.StartLog()`.
- `StartMidi()` reads the footswitches and may block for up to 10 seconds
  blinking LEDs. Call it from `main()` before `StartAudio()`.
- The DFU reset gesture is physical-only on purpose. CC 23/24 drive the
  footswitches everywhere else, but they can't reach the bootloader.
