# TriChorus

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A highly-tweakable 3-voice chorus. Capable of sounding lush and spacious like a vintage 1980s chorus, but also capable of sounding totally broken or anything in between. In this way, it's sort of like the Fuzz Factory of 3-voice choruses. With so many creative sounds available, it might take some knob twisting to find what you want, but it's totally worth it.

A 3-voice chorus works by creating 3 separate chorus voices, each with slightly varied values for LFO frequency, LFO depth, and delay time. This code allows you to dial in a "base" value for these 3 parameters, and then use the toggle switches to select how much variation you want for each parameter. Refer to the values in the table below. As an example, if **SWITCH 1** is in the **MIDDLE** position, the LFO frequencies of the 3 chorus voices would vary +/-2%, calculated as $`VariedFreq=BaseFreq*(1+.02*(VoiceNumber−1))`$. So, if the base value is 2.5Hz (set by **KNOB 2** in the noon position), the results would be:

* Voice 1: $`2.5Hz*(1+.02*(0−1))=2.45Hz`$
* Voice 2: $`2.5Hz*(1+.02*(1−1))=2.5Hz`$
* Voice 3: $`2.5Hz*(1+.02*(2−1))=2.55Hz`$

Rinse and repeat this math for DEPTH and DELAY.

> [!TIP]
> Since these values are used as computational factors, it's easy to create ugly sounds—particularly with higher values for FREQ, DEPTH, and DELAY. Start with all the switches DOWN and experiment, and/or tweak the hard-coded values to your liking.

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | VOLUME | Global output level |
| KNOB 2 | FREQ | The "base" freq for all voices' LFOs (0.2Hz to 10Hz on a parabolic curve, so noon(ish) is 2.5Hz) |
| KNOB 3 | DEPTH | The "base" depth for all voices' LFOs |
| KNOB 4 | DELAY | The "base" delay time for all voices (1ms to 30ms) |
| KNOB 5 | FEEDBACK | Feedback value for all voices |
| KNOB 6 | WET | Controls level of wet signal mixed with dry |
| SWITCH 1 | FREQ VARIATION | **UP**: +/-5%<br/>**MIDDLE**: +/-2%<br/>**DOWN**: +/-1% |
| SWITCH 2 | DEPTH VARIATION | **UP**: +/-20%<br/>**MIDDLE**: +/-10%<br/>**DOWN**: +/-5% |
| SWITCH 3 | DELAY VARIATION | **UP**: +/-20%<br/>**MIDDLE**: +/-10%<br/>**DOWN**: +/-5% |
| FOOTSWITCH 1 | Reset | If held down for 2 seconds, the Daisy Seed will reset to bootloader mode (DFU) |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
