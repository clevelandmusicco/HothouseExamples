# Venus Hothouse - Hardware Controls Reference

## Overview
Venus Hothouse provides 6 knobs, 3 toggle switches, and 2 footswitches for real-time control of spectral reverb parameters. This document details every control and its effect on the sound.

## Knob Controls

### KNOB 1: Decay
**Function**: Reverb decay time  
**Range**: 1 to 100 (arbitrary units representing decay rate)  
**Curve**: Linear  
**Default**: 10

**Description**:
Controls how long the reverb tail sustains. Lower values create shorter, room-like reverbs. Higher values create long, ambient decay that can approach infinite sustain when combined with high shimmer settings.

**Technical**: Affects the decay factor applied to reverb_energy array each processing cycle. Value is inverted (1/vdecay) so higher knob values = slower decay.

**Usage Tips**:
- **1-20**: Short room reverbs, good for drums
- **20-50**: Medium halls and plates
- **50-80**: Long ambient reverbs
- **80-100**: Very long tails, near-infinite with freeze

---

### KNOB 2: Mix
**Function**: Dry/wet balance  
**Range**: 0.0 (fully dry) to 1.0 (fully wet)  
**Curve**: Linear  
**Default**: 0.5

**Description**:
Blends the original dry signal with the processed wet reverb signal. At 0, only the dry signal passes through (reverb muted). At 1.0, only the reverb is heard.

**Technical**: Direct linear mix: `output = wet * vmix + dry * (1.0 - vmix)`

**Usage Tips**:
- **0.0-0.3**: Subtle reverb for live playing
- **0.3-0.6**: Balanced mix for most applications
- **0.6-0.9**: Emphasis on reverb character
- **0.9-1.0**: Reverb-only (use with caution in mix)

---

### KNOB 3: Damp
**Function**: High-frequency damping  
**Range**: 0.0 to 1.0  
**Curve**: Exponential (squared)  
**Default**: 0.1

**Description**:
Controls how quickly high frequencies decay in the reverb tail. Lower values create dark, muffled reverbs. Higher values preserve bright, sparkling high-end longer.

**Technical**: Above the damp threshold (as a ratio of FFT size), frequency bins are attenuated by `vdamp * fft_size/fft_bin`. The exponential curve makes the control more sensitive at low values.

**Usage Tips**:
- **0.0-0.2**: Very dark, vintage chamber sound
- **0.2-0.5**: Natural room damping
- **0.5-0.8**: Bright, modern reverb
- **0.8-1.0**: Crystalline, preserves all highs

**Note**: This control is affected by drift modulation when enabled (Toggle 3).

---

### KNOB 4: Shimmer
**Function**: Octave generation amount  
**Range**: 0.0 to 0.1  
**Curve**: Linear  
**Default**: 0.0 (off)

**Description**:
Controls the amount of octave shifting applied to the reverb. Creates ethereal, choir-like textures by morphing reverb energy to higher or lower octaves (depending on Toggle 1 position).

**Technical**: Uses exponential scaling (pow(8, vshimmer) - 1) to convert to rate per second, then calculates energy transfer between FFT bins. Energy is spread across adjacent bins for smooth transitions.

**Usage Tips**:
- **0.0**: No shimmer (standard reverb)
- **0.01-0.03**: Subtle octave hints
- **0.04-0.07**: Noticeable shimmer effect
- **0.08-0.1**: Heavy shimmer, can self-oscillate with long decay

**Interaction**: Works with Toggle 1 to determine octave direction. Affected by drift when enabled.

---

### KNOB 5: Shimmer Tone
**Function**: Fifth harmonic generation  
**Range**: 0.0 to 0.3  
**Curve**: Linear  
**Default**: 0.0 (off)

**Description**:
Adds perfect fifth (octave + fifth, i.e., 3x frequency) harmonics to the shimmer effect. Creates richer, more complex harmonic structures. Independent of KNOB 4's octave control.

**Technical**: Morphs reverb energy by a factor of 3x frequency (triple), spreading energy across 5 adjacent bins. Calculated independently from octave shimmer, allowing both to be used simultaneously.

**Usage Tips**:
- **0.0**: Pure octave shimmer only
- **0.05-0.15**: Subtle fifth harmonics
- **0.15-0.25**: Balanced octave + fifth
- **0.25-0.3**: Emphasis on fifth, organ-like

**Note**: Works best when KNOB 4 (Shimmer) is also engaged. Affected by drift modulation.

---

### KNOB 6: Detune
**Function**: Pitch shift amount and direction  
**Range**: -0.15 to +0.15 semitones (±15 cents)  
**Curve**: Linear, center-detent  
**Default**: 0.0 (center, no detune)

**Description**:
Shifts the pitch of the reverb tail up or down by small amounts, creating chorus-like detuning effects. The control has a deadzone at center (knob at noon) where no detuning occurs.

**Technical**: 
- **Center ±10% deadzone**: No detuning (detune_mode = 1)
- **Left of center**: Detune down (detune_mode = 0, multiplier = -1)
- **Right of center**: Detune up (detune_mode = 2, multiplier = 1)
- Energy is spread across 3 adjacent bins (i, i±1, i±2, i±3) based on direction

**Usage Tips**:
- **Center (11-1 o'clock)**: No detune (clean reverb)
- **Slight left/right**: Subtle chorus widening (~5 cents)
- **Medium detune**: Noticeable pitch shift (~10 cents)
- **Full left/right**: Maximum detune (±15 cents)

**Note**: Affected by drift modulation when enabled. Detune is most noticeable with longer decay times.

---

## Toggle Switch Controls

### TOGGLE 1: Shimmer Mode
**Positions**: 3 (DOWN, MIDDLE, UP)  
**Default**: MIDDLE (Octave Up)

**IMPORTANT**: Hothouse toggle switches have inverted logic compared to Funbox. Physical positions map as:
- **Physical DOWN** = Software case 2 (TOGGLESWITCH_DOWN)
- **Physical MIDDLE** = Software case 1 (TOGGLESWITCH_MIDDLE)
- **Physical UP** = Software case 0 (TOGGLESWITCH_UP)

#### Position Descriptions:

**DOWN (case 2)**: Octave Down (shimmer_mode = 0)
- Shifts reverb energy down by one octave (half frequency)
- Creates deep, sub-bass rumble effects
- Energy spread across bins i/2-1, i/2, i/2+1
- Works best with brighter source material

**MIDDLE (case 1)**: Octave Up (shimmer_mode = 1)
- Shifts reverb energy up by one octave (double frequency)
- Classic shimmer reverb sound
- Creates ethereal, choir-like textures
- Energy spread across bins 2i-1, 2i, 2i+1

**UP (case 0)**: Both Octaves (shimmer_mode = 2)
- Applies both octave up AND octave down simultaneously
- Creates complex, rich harmonic structure
- Can produce dense, chorus-like textures
- Most CPU-intensive shimmer mode

**Usage Tips**:
- Use DOWN for bass-heavy material (guitars, synths)
- Use MIDDLE for classic ambient shimmer (pads, strings)
- Use UP for maximum shimmer complexity (vocals, leads)

---

### TOGGLE 2: Reverb Character
**Positions**: 3 (DOWN, MIDDLE, UP)  
**Default**: MIDDLE (Normal)

#### Position Descriptions:

**DOWN (case 2)**: Less Lofi (reverb_mode = 0)
- Sample rate reduction: 0.3x
- Low-pass filter: 8000 Hz
- Slightly degraded, vintage character
- Maintains clarity while adding analog warmth

**MIDDLE (case 1)**: Normal (reverb_mode = 1)
- No sample rate reduction
- No additional filtering
- Clean, full-bandwidth spectral reverb
- Maximum clarity and detail

**UP (case 0)**: More Lofi (reverb_mode = 2)
- Sample rate reduction: 0.2x
- No low-pass filtering
- Heavy degradation, aliasing artifacts
- Vintage tape/vinyl character

**Technical**: Sample rate reduction is applied to the STFT output before mixing. Lofi modes introduce intentional aliasing and bandwidth limiting for creative character.

**Usage Tips**:
- Use DOWN for subtle vintage warmth
- Use MIDDLE for modern, clean reverb
- Use UP for extreme lofi, degraded textures
- Try lofi modes with shimmer for unique textures

---

### TOGGLE 3: Drift Mode
**Positions**: 3 (DOWN, MIDDLE, UP)  
**Default**: MIDDLE (No Drift)

**Description**: Drift adds slow, automated modulation to multiple parameters simultaneously, creating evolving, organic textures.

#### Position Descriptions:

**DOWN (case 2)**: Slower Drift (drift_mode = 0)
- 4 sine wave LFOs at 0.009-0.012 Hz
- Gentle, smooth parameter changes
- Subtle evolution over 90-120 second cycles
- Modulates: Damp, Shimmer, Shimmer Tone, Detune

**MIDDLE (case 1)**: No Drift (drift_mode = 1)
- All drift LFOs disabled
- Static parameter control
- Knobs have direct 1:1 response
- CPU-efficient mode

**UP (case 0)**: Faster Drift (drift_mode = 2)
- 4 triangle wave LFOs at 0.020-0.035 Hz
- More active, rhythmic modulation
- Evolution over 30-50 second cycles
- Modulates same parameters as slow drift

**Technical Details**:
When drift is active:
- **Damp**: `vdamp = vdamp * abs(drift1) * 0.7 + 0.3`
- **Shimmer**: `vshimmer *= abs(drift2)`
- **Shimmer Tone**: `vshimmer_tone *= abs(drift3)`
- **Detune**: `vdetune *= abs(drift4)`

Four independent oscillators prevent phase-locking and create complex, non-repeating patterns.

**Usage Tips**:
- Use MIDDLE for precise, consistent control
- Use DOWN for slowly evolving ambient pads
- Use UP for more active, modulated textures
- Drift works best with medium-to-long decay settings

---

## Footswitch Controls

### FOOTSWITCH 1: Bypass Toggle
**Type**: Momentary switch (latching behavior)  
**Function**: Toggles effect bypass on/off  
**LED**: LED 1 (red) - ON when bypassed, OFF when active

**Description**:
Press to toggle between bypass (dry signal only) and active (reverb processing). When bypassed, the input signal passes through unaffected with minimal latency.

**Technical**: True bypass implementation - when bypassed, audio callback passes input directly to output without DSP processing, saving CPU cycles.

**Usage**:
- Press once to activate reverb
- Press again to bypass
- LED indicates current state (lit = bypassed)

**Special Function**: Hold for 2 seconds to enter DFU bootloader mode (LEDs will flash alternately 3 times before reset).

---

### FOOTSWITCH 2: Freeze
**Type**: Momentary switch (non-latching)  
**Function**: Freezes reverb tail  
**LED**: LED 2 (red) - ON while frozen, OFF when released

**Description**:
While held, freezes the current reverb tail by:
1. Preventing new audio from entering the reverb
2. Stopping the reverb decay process
3. Maintaining current reverb energy indefinitely

**Technical**: When freeze is active:
- Input signal is still written to STFT but reverb_energy array is not updated
- Decay factor is not applied to existing reverb
- Shimmer/detune processing continues on frozen content

**Usage Tips**:
- Hold to sustain reverb indefinitely
- Great for creating drone textures
- Use with shimmer for evolving frozen textures
- Combine with long decay for smooth freeze/release transitions

**Creative Uses**:
- Freeze a chord, play melody over it
- Capture transients, release for rhythmic effects
- Create frozen pad layers
- Build complex textures with multiple freeze/release cycles

---

## LED Indicators

### LED 1 (Left, Red)
**Function**: Bypass status  
**States**:
- **ON (lit)**: Effect is bypassed (dry signal only)
- **OFF (dark)**: Effect is active (processing audio)

### LED 2 (Right, Red)
**Function**: Freeze status  
**States**:
- **ON (lit)**: Reverb is frozen (while holding Footswitch 2)
- **OFF (dark)**: Normal reverb operation

**Note**: Both LEDs are red on Hothouse (original Funbox had red and green).

---

## Parameter Interaction Matrix

| Control | Affects | Notes |
|---------|---------|-------|
| Decay + Shimmer | Can create infinite feedback | Use carefully above 80 decay with shimmer > 0.05 |
| Shimmer + Shimmer Tone | Independent but complementary | Both can be used simultaneously |
| Detune + Drift | Drift modulates detune amount | Creates evolving chorus effects |
| Damp + Drift | Drift modulates damp value | Subtle evolution of tone |
| Lofi Mode + Shimmer | Shimmer is lofi-processed | Creates degraded shimmer character |
| Freeze + Decay | Decay has no effect while frozen | Reverb tail is held indefinitely |
| Freeze + Shimmer | Shimmer continues on frozen tail | Creates evolving frozen textures |

---

## Performance Tips

### CPU Load Management
- Drift modes add minimal CPU overhead
- More Lofi mode is actually less CPU intensive
- Octave Up+Down shimmer uses more CPU than single octave
- Freeze slightly reduces CPU load (no decay calculations)

### Musical Applications

**Ambient Pads**:
- Long decay (70-90)
- Medium shimmer (0.04-0.06)
- Subtle shimmer tone (0.1-0.15)
- Slow drift enabled
- Mix 70-90%

**Rhythm Processing**:
- Short-medium decay (20-40)
- No shimmer
- Less lofi mode
- No drift
- Mix 30-50%

**Textural Soundscapes**:
- Very long decay (90-100)
- High shimmer (0.07-0.1)
- High shimmer tone (0.2-0.3)
- Fast drift
- Use freeze for layering
- Mix 80-100%

**Guitar/Synth Lead**:
- Medium decay (40-60)
- Light shimmer (0.02-0.04)
- Subtle detune (5-10 cents)
- Normal mode
- Mix 40-60%

---

## Default Settings Reference

| Parameter | Default Value | Notes |
|-----------|--------------|-------|
| Decay (K1) | 10 | Short reverb |
| Mix (K2) | 0.5 | Equal blend |
| Damp (K3) | 0.1 | Dark character |
| Shimmer (K4) | 0.0 | No shimmer |
| Shimmer Tone (K5) | 0.0 | No fifth |
| Detune (K6) | 0.0 | No detune |
| Shimmer Mode (T1) | MIDDLE | Octave up |
| Reverb Mode (T2) | MIDDLE | Normal |
| Drift Mode (T3) | MIDDLE | No drift |
| Bypass | ON | Bypassed on startup |
| Freeze | OFF | Not frozen |

---

## Quick Reference Card

```
┌─────────────────────────────────────────────────────────┐
│              VENUS HOTHOUSE CONTROLS                     │
├─────────────────────────────────────────────────────────┤
│ KNOBS:                                                  │
│  K1: Decay (1-100)        K4: Shimmer (0-0.1)          │
│  K2: Mix (0-1)            K5: Shimmer Tone (0-0.3)     │
│  K3: Damp (0-1, exp)      K6: Detune (±0.15)           │
│                                                         │
│ TOGGLES:                                                │
│  T1: Shimmer Mode (Down/Up/Both)                       │
│  T2: Reverb Character (Less Lofi/Normal/More Lofi)     │
│  T3: Drift Speed (Slow/Off/Fast)                       │
│                                                         │
│ FOOTSWITCHES:                                           │
│  FS1: Bypass Toggle (LED 1 = bypassed)                │
│  FS2: Freeze (hold, LED 2 = frozen)                   │
└─────────────────────────────────────────────────────────┘
```
