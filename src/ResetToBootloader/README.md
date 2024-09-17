# ResetToBootloader

Contributed by Cleveland Music Co. <code@clevelandmusicco.com>

## Description

A very short example to demonstrate a simple (quick and dirty?) approach to programmatically putting the Daisy Seed into bootloader mode so it can be flashed with `dfu-util` over USB. The result is the same as pressing the BOOT and RESET buttons on the Daisy Seed as described [here](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4a-flashing-the-daisy-via-usb).

In this code, holding down `FOOTSWITCH_1` for one second will briefly flash `LED_1` three times and reset the Daisy Seed by calling `System::ResetToBootloader()`. This can be handy while doing development work, as there's no need to physically press the Daisy Seed buttons; you can quickly prepare to flash by holding down the left footswitch.

> [!TIP]
> Think carefully about leaving such code enabled in your final program; it could be too easy to mistakenly reset the Hothouse (for example, in a live performance situation!), requiring you to power cycle to recover. Consider the use of a compile-time flag (something like `#ifdef DEVELOPMENT`), or a runtime flag (like `enable_bootloader_reset = true;`) to disable this functionality for "production" builds.
>
> You could also trigger a reset with a more complex set of user interactions with the knobs and switches, but this may not be convenient at development time.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | Unused |  |
| KNOB 2 | Unused |  |
| KNOB 3 | Unused |  |
| KNOB 4 | Unused |  |
| KNOB 5 | Unused |  |
| KNOB 6 | Unused |  |
| SWITCH 1 | Unused |  |
| SWITCH 2 | Unused |  |
| SWITCH 3 | Unused |  |
| FOOTSWITCH 1 | Reset | Holding down for one second will flash LED_1 three times and put the Daisy Seed in bootloader mode |
| FOOTSWITCH 2 | Bypass | When NOT bypassed, zeroes are sent to the left output; when bypassed, output is the same as the input |
