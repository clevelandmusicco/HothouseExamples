# TriChorus

Contributed by Cleveland Music Co. \<<code@clevelandmusicco.com>\>

## Description

A highly-tweakable 3-voice chorus. Capable of sounding lush and spacious like a vintage 1980s chorus, but also capable of sounding totally broken ... or any reasonable step in between. In this way, it's sort of like the Fuzz Factory of 3-voice choruses.

A 3-voice chorus works by creating 3 separate chorus voices, each with slightly varied values for LFO frequency, LFO depth, and delay time. This code allows you to dial in a "base" value for these 3 parameters, and then use the toggle switches to select how much variation you want for each parameter.

Refer to the values in the table below. As an example, if SWITCH 1 is DOWN, the LFO frequencies of the 3 chorus voices would vary +/-2%. Thus, if the base value is 5Hz (set by KNOB 2), the result would be:

* Voice 1 - 4.9Hz LFO freq
* Voice 2 - 5Hz LFO freq
* Voice 3 - 5.1Hz LFO freq

Rinse and repeat this math for DEPTH and DELAY.

> [!WARNING]
> Since these variation values are used as computational factors, it's easy to create ugly sounds. Starting with all the switches DOWN is recommended. Experiment, experiment, experiment!

### Controls

| CONTROL | DESCRIPTION | NOTES |
|-|-|-|
| KNOB 1 | VOLUME | Global output level |
| KNOB 2 | FREQ | The "base" freq for all voices' LFOs |
| KNOB 3 | DEPTH | The "base" depth for all voices' LFOs |
| KNOB 4 | DELAY | The "base" delay time for all voices |
| KNOB 5 | FEEDBACK | Feedback value for all voices |
| KNOB 6 | WET | Controls level of wet signal mixed with dry |
| SWITCH 1 | FREQ VARIATION | **UP**: +/-20%<br/>**MIDDLE**: +/-10%<br/>**DOWN**: +/-2% |
| SWITCH 2 | DEPTH VARIATION | **UP**: +/-50% (!!)<br/>**MIDDLE**: +/-25%<br/>**DOWN**: +/-5% |
| SWITCH 3 | DELAY VARIATION | **UP**: +/-20%<br/>**MIDDLE**: +/-10%<br/>**DOWN**: +/-5% |
| FOOTSWITCH 1 | Unused |  |
| FOOTSWITCH 2 | Bypass | The bypassed signal is buffered |
