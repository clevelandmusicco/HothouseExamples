# Interplay

Contributed by Jon Philpott \<<jon.philpott@gmail.com>\>

## Description

An evolution of the frippertronics effect but two fundamental changes.

1. The delay time now has granular settings between 800ms - 30s
   instead of 4 discrete settings.
2. the offset between the left and right delay times is now the golden
   ration (1.618) - this gives a similiar co-primeish feel without
   having to have static values (see #1)
   
I've spent quite a long time tweaking all the values to get the
"tape-feel" sounding right to my ears. 

It features two long-ish, modulated tape-like delay lines with
controls to alter the feedback of each individual delay and the amount
of cross feedback between the delays.

The foot switches are used in a momentary fashion, input into the
delay lines only happens while you are holding down the first foot
switch whereas the second foot switch is used to erase the currently
heard sounds from entering the delay lines again, a double-tap erases
the entire loop.

I built this pedal originally using my own hardware, but decided to
bring it to the opensource community via Hothouse, you can see the
manual for my hardware here that explains the effect better (plus it
looks kinda cool!) https://fezmaster.com/interplay.pdf



### Controls

| CONTROL      | DESCRIPTION                 | NOTES                                                                                                |
|--------------|-----------------------------|------------------------------------------------------------------------------------------------------|
| KNOB 1       | "Impermanence" - feedback   | feedback amount for each loop                                                                        |
| KNOB 2       | "Time Basis" - loop time    | delay time from 800ms - 30 sec. second delay will be 1.618 times longer                              |
| KNOB 3       | "Not-self" - cross-feedback | the amount that each delay feeds the other delay                                                     |
| KNOB 4       | Noise level                 | adjustable level of "tape hiss"                                                                      |
| KNOB 5       | Wet/Dry Mix                 | balance between dry and delay sounds                                                                 |
| KNOB 6       | Unused                      |                                                                                                      |
| SWITCH 1     | Mono/Stereo Mode            | **UP** - Stereo<br/>**MIDDLE** - Mono <br/>**DOWN** - Mono                                           |
| SWITCH 2     | Delay Enable                | **UP** first delay only heard <br/>**MIDDLE** both delays heard <br/>**DOWN** only right delay heard |
| SWITCH 3     | Unused                      | **UP** - <br/>**MIDDLE** - <br/>**DOWN** -                                                           |
| FOOTSWITCH 1 | "integrate" - record        | Momentary: when held audio is recorded into the loops                                                |
| FOOTSWITCH 2 | "release" - erase           | Momentary: when held audio is prevented from entering the loops again. Double tap to erase all audio |
|              |                             |                                                                                                      |


