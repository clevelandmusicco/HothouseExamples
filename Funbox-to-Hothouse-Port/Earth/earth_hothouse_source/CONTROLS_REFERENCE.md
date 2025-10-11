# Earth Reverbscape - Controls Reference

Comprehensive documentation of all hardware controls and their functions for the Earth Reverbscape Hothouse port.

## Control Layout

```
        KNOB 1    KNOB 2    KNOB 3    KNOB 4    KNOB 5    KNOB 6
       PreDelay    Mix      Decay    ModDepth  ModSpeed  Damping

        TOGGLE 1          TOGGLE 2          TOGGLE 3
       Time Scale       Effect Mode      FS2 Function
       [UP|MID|DN]      [UP|MID|DN]      [UP|MID|DN]

          LED 1              LED 2
        (Bypass)          (FS2 Active)

       FOOTSWITCH 1      FOOTSWITCH 2
      (Bypass Toggle)   (Function Mode)
```

## Detailed Knob Controls

### KNOB 1: Reverb Pre-delay

**Function:** Controls the delay time before reverb processing begins

**Range:** 0ms to 100ms  
**Curve:** Linear  
**Default:** 50ms (center position)

**Description:**
Pre-delay creates space between the direct signal and the reverb tail, simulating the time it takes for sound to reach the first reflective surface in a real space. Short pre-delays (0-20ms) create immediate, intimate ambience. Medium pre-delays (20-50ms) add clarity and separation. Long pre-delays (50-100ms) create distinct rhythmic echoes before the reverb tail.

**Musical Applications:**
- **0-10ms:** Tight ambience, good for thick textures
- **10-30ms:** Clear vocal/instrument presence with reverb
- **30-60ms:** Rhythmic slapback before reverb tail
- **60-100ms:** Distinct echo effect leading into reverb

**Technical Notes:**
Pre-delay is smoothed with a one-pole filter (0.0002 coefficient) to prevent clicks when adjusting. The Dattorro algorithm handles pre-delay internally with high-precision interpolation.

---

### KNOB 2: Dry/Wet Mix

**Function:** Balances the clean (dry) signal with the affected (wet) signal

**Range:** 100% dry to 100% wet  
**Curve:** Constant-power crossfade (maintains perceived loudness)  
**Default:** 50/50 mix (center position)

**Description:**
The mix control uses a sophisticated constant-power crossfade algorithm that maintains consistent perceived loudness as you blend between dry and wet signals. This prevents the "hole in the middle" problem common with simple linear mixing.

**Mix Formula:**
```
x2 = 1.0 - vmix
A = vmix * x2
B = A * (1.0 + 1.4186 * A)
C = B + vmix
D = B + x2
wetMix = C * C
dryMix = D * D
```

**Musical Applications:**
- **Fully CCW (0%):** Dry signal only, useful for A/B comparison
- **25%:** Subtle ambience, dry sound remains dominant
- **50% (center):** Balanced mix, classic reverb sound
- **75%:** Wet-dominated, great for ambient pads
- **Fully CW (100%):** Reverb only, no dry signal (ambient/experimental)

**Technical Notes:**
The constant-power formula ensures equal perceived loudness at all mix positions. The algorithm squares the final mix values (C and D) to create smooth crossfade curves.

---

### KNOB 3: Reverb Decay

**Function:** Controls how long the reverb tail sustains

**Range:** 0.0 (no decay) to 1.0 (infinite sustain)  
**Curve:** Linear  
**Default:** 0.5 (moderate decay)

**Description:**
Decay controls the feedback amount in the reverb tank. Lower values create short, tight reverbs. Higher values create long, lush tails. At maximum, the reverb approaches infinite sustain (freeze).

**Musical Applications:**
- **0.0-0.3:** Short room/plate (1-2 seconds)
- **0.3-0.5:** Medium hall (2-4 seconds)
- **0.5-0.7:** Large hall/cathedral (4-8 seconds)
- **0.7-0.9:** Very long tails (8-15 seconds)
- **0.9-1.0:** Near-infinite/freeze (15+ seconds)

**Interaction with TOGGLE 3:**
When Freeze mode is engaged (TOGGLE 3 UP + FOOTSWITCH 2 held), decay is automatically ramped to 1.0 regardless of knob position. When released, it smoothly returns to the knob setting.

**Technical Notes:**
Decay is smoothed with a one-pole filter (0.0002 coefficient) for smooth transitions. The freeze function uses a slower ramp (same coefficient) to create smooth decay transitions when engaging/releasing.

---

### KNOB 4: Modulation Depth

**Function:** Controls the intensity of pitch modulation in the reverb tank

**Range:** 0.0 (no modulation) to 8.0 (deep modulation)  
**Curve:** Linear, scaled internally  
**Default:** 0.1 (subtle modulation)

**Description:**
Modulation depth controls how far the pitch varies in the reverb tank. This creates a subtle chorus-like effect that adds life and movement to the reverb tail. The Dattorro algorithm applies this modulation to the tank delay lines.

**Musical Applications:**
- **0.0:** Static, pristine reverb (no modulation)
- **0.1-2.0:** Subtle shimmer, barely perceptible movement
- **2.0-4.0:** Noticeable chorus, lush and animated
- **4.0-6.0:** Strong modulation, obvious pitch variation
- **6.0-8.0:** Deep modulation, dramatic wobble effect

**Interaction with KNOB 5:**
Modulation depth and speed work together. Deep, slow modulation creates a gentle swell. Deep, fast modulation creates vibrato or shimmer effects.

**Technical Notes:**
The knob value (0.0-1.0) is multiplied by 8 before being sent to the Dattorro algorithm. Modulation is smoothed with a one-pole filter (0.0002 coefficient) to prevent zipper noise.

---

### KNOB 5: Modulation Speed

**Function:** Controls the rate of pitch modulation in the reverb tank

**Range:** 0.3 Hz to 15.3 Hz  
**Curve:** Linear, scaled internally  
**Default:** 8.15 Hz (moderate speed)

**Description:**
Modulation speed controls how fast the pitch varies in the reverb tank. Slow speeds create gentle, sweeping motion. Fast speeds create shimmer or vibrato effects.

**Speed Formula:**
```
speed = 0.3 + (knob_value * 15.0) Hz
```

**Musical Applications:**
- **0.3-2.0 Hz:** Slow swell, subtle movement
- **2.0-5.0 Hz:** Gentle chorus, natural ambience
- **5.0-8.0 Hz:** Moderate shimmer, lively reverb
- **8.0-12.0 Hz:** Fast shimmer, obvious modulation
- **12.0-15.3 Hz:** Very fast, vibrato-like effect

**Interaction with KNOB 4:**
Speed determines how quickly you hear the depth setting. Slow speed with deep depth creates long swells. Fast speed with deep depth creates rapid pitch variation.

**Technical Notes:**
The knob value (0.0-1.0) is scaled to 0.3-15.3 Hz. Modulation is smoothed with a one-pole filter (0.0002 coefficient). The Dattorro algorithm uses LFOs to modulate delay line read positions.

---

### KNOB 6: Damping (High-Cut Filter)

**Function:** Controls the high-frequency damping in the reverb

**Range:** Dark (10.0 - 7.0 = 3.0) to Bright (10.0)  
**Curve:** Inverse linear  
**Default:** 6.5 (moderate damping)

**Description:**
Damping controls the high-frequency rolloff in the reverb tail, simulating how real spaces absorb high frequencies over time. Lower values create darker, warmer reverbs. Higher values create brighter, more present reverbs.

**Damping Formula:**
```
highCutPitch = 10.0 - (7.0 * knob_value)
```

**Musical Applications:**
- **Fully CCW (dark):** Warm, vintage plate/spring sound
- **25%:** Dark hall, muted high end
- **50% (center):** Balanced, natural room sound
- **75%:** Bright plate, preserved high frequencies
- **Fully CW (bright):** Brilliant, modern digital reverb

**Technical Notes:**
The damping value is converted to a pitch value for the Dattorro input filter. Lower pitch values result in lower cutoff frequencies (darker sound). The inverse relationship means turning the knob clockwise brightens the reverb.

---

## Toggle Switch Controls

### TOGGLE 1: Reverb Time Scale

**Function:** Multiplies the reverb decay time by different factors

**Positions:**

**UP (Physical) → Case 0 (Code) → 1.0x Time Scale:**
- Normal reverb length
- Tight, controlled tails
- Good for musical playing
- Best for rhythm guitar, tight mixes

**MIDDLE (Physical) → Case 1 (Code) → 2.0x Time Scale:**
- Extended reverb length (default)
- Lush, sustained tails
- Good for ambient textures
- Best for pads, sustained notes

**DOWN (Physical) → Case 2 (Code) → 4.0x Time Scale:**
- Very long reverb length
- Massive, overwhelming tails
- Experimental/ambient use
- Best for drones, soundscapes

**Musical Applications:**

**1.0x (UP):**
Use for situations where you need clear, defined reverb that doesn't overwhelm the mix. Perfect for rhythm guitar, funk, or any style where clarity is important.

**2.0x (MIDDLE):**
The sweet spot for most applications. Provides lush, sustained reverb tails without becoming overwhelming. Great for leads, pads, and ambient guitar.

**4.0x (DOWN):**
Extreme reverb times that can sustain nearly indefinitely at high decay settings. Use for experimental ambient textures, drones, or when you want the reverb to become a dominant element.

**Interaction with KNOB 3 (Decay):**
Time scale multiplies the effective decay time. At 2.0x scale and 0.5 decay, you get the same tail length as 1.0x scale and higher decay. Use time scale for overall character, decay for fine-tuning.

**Technical Notes:**
Time scale is applied directly to the Dattorro algorithm's internal delay times. Changes are applied immediately when the switch position changes.

---

### TOGGLE 2: Effect Mode (Octave Configuration)

**Function:** Selects which octave effects are active

**Positions:**

**UP (Physical) → Case 0 (Code) → Reverb Only:**
- No octave generation
- Cleanest sound
- Lowest CPU usage (~40%)
- Dry signal goes straight to reverb

**MIDDLE (Physical) → Case 1 (Code) → Reverb + Octave Up:**
- One octave above input pitch
- Shimmer reverb effect
- Medium CPU usage (~60%)
- Popular modern reverb sound

**DOWN (Physical) → Case 2 (Code) → Reverb + Full Octaves:**
- Octave up + Octave down + Sub-octave
- Massive, thick harmonization
- Highest CPU usage (~65%)
- Experimental ambient tones

**Musical Applications:**

**Reverb Only (UP):**
Use when you want pure, clean reverb without pitch effects. Best for traditional reverb applications where you need clarity and definition.

**Octave Up (MIDDLE):**
Creates the popular "shimmer reverb" effect. The octave-up harmonics blend with the reverb to create ethereal, heavenly textures. Perfect for ambient guitar, pads, and atmospheric sounds.

**Full Octaves (DOWN):**
Adds octave up, octave down, and sub-octave simultaneously. Creates massive, thick harmonic content. Best for experimental textures, drones, or when you want overwhelming sonic density.

**Technical Implementation:**

The octave generator uses multirate processing:
1. Input is buffered (6 samples)
2. Decimated 6:1 for pitch tracking (8kHz processing rate)
3. Octave harmonics generated
4. Interpolated back to 48kHz
5. EQ compensation applied (highshelf -11dB @ 140Hz, lowshelf +5dB @ 160Hz)
6. Mixed with dry signal (50% dry level)

**Octave Mix Levels:**
- Octave up: 2.0x gain
- Octave down: 2.0x gain (when active)
- Sub-octave: 2.0x gain (when active)

**Technical Notes:**
Octave processing only occurs when bin_counter reaches 5 (every 6 samples). Buffer management is critical to avoid clicks. The EQ compensation helps balance the octave harmonics with the fundamental.

---

### TOGGLE 3: Footswitch 2 Function Mode

**Function:** Selects what FOOTSWITCH 2 does when held

**Positions:**

**UP (Physical) → Case 0 (Code) → Freeze Mode:**
- FOOTSWITCH 2 freezes the reverb tail
- Decay ramped to 1.0 (infinite)
- Wet signal reduced 40% to prevent build-up
- LED 2 on while frozen

**MIDDLE (Physical) → Case 1 (Code) → Overdrive Boost:**
- FOOTSWITCH 2 engages overdrive
- Drive ramped from 0.4 to 0.6
- Smooth swell/fade with volume compensation
- LED 2 on while overdrive active

**DOWN (Physical) → Case 2 (Code) → Momentary Effect:**
- FOOTSWITCH 2 toggles effect on/off momentarily
- Effect active only while held
- LED 2 on while effect active
- Returns to bypassed when released

**Musical Applications:**

**Freeze Mode (UP):**
Hold FOOTSWITCH 2 to freeze the current reverb tail infinitely. The tail continues to circulate while frozen. Release to return to normal decay. The 40% reduction prevents excessive build-up while maintaining the frozen sound. Perfect for:
- Creating ambient pads under playing
- Building layers of harmonic content
- Experimental soundscapes
- "Freeze + play" techniques

**Overdrive Boost (MIDDLE):**
Hold FOOTSWITCH 2 to smoothly swell into overdrive. The drive level ramps from 0.4 to 0.6 with automatic volume compensation. The original "bloom/fade" formula creates an artistic swelling effect. Perfect for:
- Adding grit to reverb tails
- Creating dynamics in ambient passages
- Distorted reverb textures
- Controllable saturation

**Momentary Effect (DOWN):**
Hold FOOTSWITCH 2 to temporarily engage the effect. Release to return to bypass. Useful for:
- Spot reverb effects
- Experimental performance techniques
- Controlled effect application
- Dynamic expression

**Overdrive Volume Compensation:**

The overdrive stage uses the original artistic volume compensation formula:
```
compensation = 1.0f - (drive * drive * 2.8f - 0.1296f)
```

This creates a bloom/fade effect rather than flat volume compensation. At drive = 0.4, compensation = 1.0064 (slight boost). At drive = 0.6, compensation = 0.8784 (reduction). This is intentional and creates the characteristic swell.

**Technical Notes:**
The mode selection affects only the behavior of FOOTSWITCH 2. All three modes use smooth ramping with one-pole filters to prevent clicks. The freeze mode uses a slower ramp for decay (0.0002 coefficient). The overdrive uses an even slower ramp (0.000015 coefficient) to create the characteristic swell effect.

---

## Footswitch Controls

### FOOTSWITCH 1: Bypass Toggle

**Function:** True bypass toggle with LED indication

**Operation:**
- **Press:** Toggle between bypassed and active
- **LED 1 OFF:** Effect bypassed (dry signal only)
- **LED 1 ON (red):** Effect active
- **Hold 2 seconds:** Enter bootloader mode (LEDs flash)

**Bypass Behavior:**
When bypassed, the dry signal passes through unchanged. All DSP continues running in the background, so the reverb tail is maintained. This allows you to bypass and un-bypass without losing the reverb state.

**Bootloader Mode:**
Hold FOOTSWITCH 1 for 2 seconds to enter DFU (bootloader) mode. The LEDs will flash alternately 3 times before resetting. This allows firmware updates without pressing the physical BOOT button.

**Technical Notes:**
The bypass uses a rising edge detector. The LED is updated every audio callback. The bootloader timer uses System::GetNow() and checks for 2000ms hold time.

---

### FOOTSWITCH 2: Function Mode

**Function:** Momentary control whose function depends on TOGGLE 3

**Operation:**
- **Press and hold:** Activate selected function
- **LED 2 ON (red):** Function active while held
- **Release:** Deactivate function

**Functions (set by TOGGLE 3):**

**When TOGGLE 3 = UP (Freeze):**
- Hold to freeze reverb tail infinitely
- Decay ramped to 1.0, wet reduced 40%
- Release to return to normal

**When TOGGLE 3 = MIDDLE (Overdrive):**
- Hold to swell into overdrive
- Drive ramped from 0.4 to 0.6
- Volume compensation applied
- Release to fade out

**When TOGGLE 3 = DOWN (Momentary):**
- Hold to engage effect
- Release to bypass effect
- Instant on/off (no ramping)

**LED Behavior:**
LED 2 turns on immediately when FOOTSWITCH 2 is pressed and turns off when released, regardless of mode. This provides clear visual feedback of footswitch state.

**Technical Notes:**
The footswitch uses .Pressed() state rather than edge detection for continuous hold detection. The fw2_held flag tracks hold state. Ramping behavior varies by mode.

---

## LED Indicators

### LED 1 (Left)

**Color:** Red  
**Function:** Bypass state indicator

**States:**
- **OFF:** Effect bypassed (true bypass)
- **ON (red):** Effect active

**Brightness:** Fixed at 1.0 (full brightness) when on

**Technical Notes:**
Updated every audio callback with .Update(). Uses hardware pin 22 (Hothouse::LED_1).

---

### LED 2 (Right)

**Color:** Red  
**Function:** Footswitch 2 state indicator

**States:**
- **OFF:** FOOTSWITCH 2 not pressed
- **ON (red):** FOOTSWITCH 2 currently held

**Brightness:** Fixed at 1.0 (full brightness) when on

**Technical Notes:**
Directly reflects fw2_held flag state. Updated every audio callback with .Update(). Uses hardware pin 23 (Hothouse::LED_2).

---

## Parameter Interaction Matrix

This matrix shows how different controls interact with each other:

| Control | Interacts With | Effect |
|---------|---------------|--------|
| KNOB 3 (Decay) | TOGGLE 3 (Freeze) | Overridden when freeze active |
| KNOB 4 (Mod Depth) | KNOB 5 (Mod Speed) | Together create modulation character |
| TOGGLE 1 (Time Scale) | KNOB 3 (Decay) | Multiplies effective decay time |
| TOGGLE 2 (Effect Mode) | All reverb controls | Determines what feeds reverb input |
| TOGGLE 3 (FS2 Function) | FOOTSWITCH 2 | Selects footswitch behavior |
| FOOTSWITCH 2 | KNOB 3 (Decay) | Can override decay in freeze mode |

---

## Performance Tips

### Creating Shimmer Reverb
1. Set TOGGLE 2 to MIDDLE (octave up)
2. Set KNOB 3 (decay) to 0.6-0.8
3. Set KNOB 2 (mix) to 40-60%
4. Adjust KNOB 4/5 (modulation) for shimmer intensity
5. Set KNOB 6 (damping) bright for ethereal sound

### Creating Freeze Pads
1. Set TOGGLE 3 to UP (freeze mode)
2. Set KNOB 3 (decay) to 0.7+
3. Play a chord and hold FOOTSWITCH 2
4. Play over the frozen pad
5. Release and re-freeze for layering

### Creating Vintage Plate Sound
1. Set TOGGLE 1 to UP (1.0x time)
2. Set TOGGLE 2 to UP (reverb only)
3. Set KNOB 3 (decay) to 0.3-0.5
4. Set KNOB 6 (damping) dark (CCW)
5. Set KNOB 4 (mod depth) subtle (1-2)

### Creating Experimental Textures
1. Set TOGGLE 2 to DOWN (full octaves)
2. Set TOGGLE 1 to DOWN (4.0x time)
3. Set KNOB 3 (decay) to 0.8+
4. Use TOGGLE 3 freeze to build layers
5. Experiment with overdrive boost

---

## MIDI Control

Earth Reverbscape supports MIDI CC control of all six knobs:

| CC Number | Control | Function |
|-----------|---------|----------|
| CC 14 | KNOB 1 | Pre-delay |
| CC 15 | KNOB 2 | Mix |
| CC 16 | KNOB 3 | Decay |
| CC 17 | KNOB 4 | Modulation Depth |
| CC 18 | KNOB 5 | Modulation Speed |
| CC 19 | KNOB 6 | Damping |

**MIDI Override Behavior:**
When a MIDI CC is received, that knob's value is overridden by MIDI until the physical knob is moved. This allows seamless switching between MIDI and manual control without jumps.

**Expression Pedal:**
The expressionHandler supports expression pedal input via MIDI. Expression can be mapped to any parameter through the expression handler.

---

## Default Settings

On power-up or reset, Earth Reverbscape initializes with these settings:

| Control | Default Value | Description |
|---------|---------------|-------------|
| Pre-delay | 0.5 (50ms) | Center position |
| Mix | 0.5 (50/50) | Equal dry/wet |
| Decay | 0.5 | Moderate decay |
| Mod Depth | 0.1 (0.8 internal) | Subtle modulation |
| Mod Speed | 0.5 (8.15 Hz) | Moderate speed |
| Damping | 0.5 (6.5 pitch) | Balanced brightness |
| Time Scale | 2.0x | Extended reverb |
| Effect Mode | Reverb only | No octave |
| FS2 Function | Freeze | Freeze mode |
| Bypass | OFF | Bypassed on startup |

**Technical Notes:**
All parameter smoothing values are also initialized to default positions to prevent parameter jumps on first use.

---

## Quick Reference Card

```
EARTH REVERBSCAPE - QUICK REFERENCE

KNOBS:
1. Pre-delay (0-100ms)
2. Mix (Dry/Wet)
3. Decay (0-infinite)
4. Mod Depth (0-8)
5. Mod Speed (0.3-15Hz)
6. Damping (Dark-Bright)

TOGGLE 1 (Time Scale):
UP: 1.0x  |  MID: 2.0x  |  DN: 4.0x

TOGGLE 2 (Effect Mode):
UP: Reverb Only
MID: Reverb + Octave Up
DN: Reverb + All Octaves

TOGGLE 3 (FS2 Function):
UP: Freeze  |  MID: Overdrive  |  DN: Momentary

FOOTSWITCHES:
FS1: Bypass (Hold 2s = Bootloader)
FS2: Function (see TOGGLE 3)

LEDS:
LED1 (Red): Bypass State
LED2 (Red): FS2 Active
```

---

## Support

For questions about controls or usage:
- Hothouse Examples: https://github.com/clevelandmusicco/HothouseExamples
- Daisy Forum: https://forum.electro-smith.com/
- Porter: Chris Brandt (chris@futr.tv)