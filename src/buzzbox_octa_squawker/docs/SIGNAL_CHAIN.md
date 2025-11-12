# Buzzbox Octa Squawker Signal Chain Documentation

Complete technical documentation of audio signal flow and processing stages.

## Overview

Buzzbox Octa Squawker processes audio through multiple stages with flexible routing controlled by switches. Understanding the signal chain helps with sound design and troubleshooting.

## Master Signal Flow

```
Input → Stage 1 → Stage 2/3 → Stage 4 → Stage 5/6 → Stage 6.5 → Stage 7 → Stage 8 → Output
```

### Full Stage Breakdown

1. **Stage 1**: Input Gain
2. **Stage 2**: Autowah (if SW1 UP)
3. **Stage 3**: Octave Generation
4. **Stage 4**: Fuzz Processing
5. **Stage 5**: Autowah (if SW1 MIDDLE)
6. **Stage 6**: Autowah (if SW1 DOWN)
7. **Stage 6.5**: FS2 Gain Compensation
8. **Stage 7**: Dry/Wet Mix
9. **Stage 8**: Output Level

---

## Detailed Stage Documentation

### STAGE 1: Input Gain

**Function**: Scales input signal before any processing

**Formula**:
```cpp
const float input_gain = 0.5f + (knobValues[0] * 1.5f);  // 0.5x to 2.0x
signal *= input_gain;
```

**Control**: Knob 1 (always active)

**Technical Notes**:
- Applied to entire signal path
- Linear scaling
- Affects all downstream processing

---

### STAGE 2: Autowah BEFORE Fuzz

**Active When**:
- `autowah_enabled == true` AND
- `autowah_placement == 0` (Switch 1 UP)

**Processing Chain**:

1. **Envelope Detection**:
   ```cpp
   float envelope = envelopeFollower.Process(signal);
   ```
   - Attack: 5ms
   - Release: 50ms

2. **Gate/ADSR**:
   ```cpp
   float gate_level = 0.05f + (autowah_threshold * 0.15f);  // 0.05 to 0.20
   bool gate = (envelope > gate_level);
   float adsr_out = autowah_adsr.Process(gate);
   ```
   - Attack: 10-200ms (controlled by Knob 4)
   - Decay: 150ms (fixed)
   - Sustain: 0.3 (fixed)
   - Release: 20-400ms (controlled by Knob 4)

3. **Frequency Mapping**:
   ```cpp
   float range_min = 100.0f + (autowah_range * 200.0f);   // 100-300Hz
   float range_max = 1100.0f + (autowah_range * 1900.0f); // 1100-3000Hz
   float filter_freq = range_min + (adsr_out * (range_max - range_min));
   ```

4. **SVF Bandpass Filter**:
   ```cpp
   autowah_svf.SetFreq(filter_freq);
   autowah_svf.SetRes(0.7f);  // Q = 0.7 (moderate resonance)
   signal = autowah_svf.Band() * 2.0f;  // +6dB gain compensation
   ```

**Control**: Switch 3 MIDDLE position (Knobs 4-6)

---

### STAGE 3: Octave Generation

**Active When**: `octave_enabled == true`

**Processing**: Multirate (6:1 decimation)

**Decimation Chain**:
```cpp
// Buffer input (6 samples)
octave_buff[octave_bin_counter] = signal;

// Process every 6 samples
if (octave_bin_counter == 5) {
    // Decimate 48kHz → 8kHz
    std::span<const float, 6> in_chunk(&octave_buff[0], 6);
    const auto sample = decimate(in_chunk);
    
    // Octave generation at 8kHz
    octave.update(sample);
    
    // Mix octaves
    float octave_signal = octave.up1() * octave_up_level * 1.5f + 
                         octave.down1() * octave_down_level * 1.5f;
    
    // Interpolate 8kHz → 48kHz
    auto out_chunk = interpolate(octave_signal);
    
    // Mix with dry
    for (size_t j = 0; j < 6; ++j) {
        octave_buff_out[j] = octave_buff[j] * (1.0f - octave_mix) + 
                            out_chunk[j] * octave_mix;
    }
}
```

**Sample Rate**: 8kHz (decimated from 48kHz)

**Latency**: 6 samples @ 48kHz = 0.125ms

**Controls**:
- Knob 4: Octave up level (0-1, with 1.5x boost)
- Knob 5: Octave down level (0-1, with 1.5x boost)
- Knob 6: Octave mix (dry/wet)

**Technical Notes**:
- Uses Q library for pitch shifting
- Processes in 6-sample blocks
- Output interpolated back to 48kHz

---

### STAGE 4: Fuzz Processing

**Active When**: `fuzz_enabled == true` (FS1 engaged)

**Type**: Always AGGRESSIVE (asymmetric clipping)

**Processing Chain**:

1. **Bass Boost** (before clipping):
   ```cpp
   static float bass_lpf = 0.0f;
   const float bass_coeff = 0.05f;  // ~150Hz corner
   bass_lpf = bass_lpf + bass_coeff * (fuzz_signal - bass_lpf);
   fuzz_signal = fuzz_signal + (bass_lpf * 0.8f);  // 80% boost
   ```

2. **Drive/Gain**:
   ```cpp
   const float gain = 1.0f + (drive_amount * 19.0f);  // 1x to 20x
   const float intensity = drive_amount;               // 0 to 1
   fuzz_signal *= gain;
   ```

3. **Fuzz with Oversampling** (4x):
   ```cpp
   std::vector<float> oversampled = Oversampling::upsample(fuzz_signal);  // 4 samples
   for (float& sample : oversampled) {
       sample = Fuzz::process(sample, FuzzType::AGGRESSIVE, intensity);
   }
   fuzz_signal = Oversampling::downsample(oversampled);
   ```

4. **Gate** (if threshold > 0.01):
   ```cpp
   float gate_level = gate_threshold * 0.1f;
   float input_level = std::abs(signal);  // Detect on pre-fuzz signal
   
   if (input_level > gate_level) {
       gate_envelope += 0.95f * (1.0f - gate_envelope);  // Fast attack
   } else {
       gate_envelope += 0.01f * (0.0f - gate_envelope);  // Slow release
   }
   
   fuzz_signal *= gate_envelope;
   ```

5. **Tone Control** (simple lowpass):
   ```cpp
   tone.SetFreq(tone_freq);  // 100Hz to 1500Hz
   fuzz_signal = tone.Process(fuzz_signal);
   ```

**Controls** (Switch 3 UP):
- Knob 4: Drive (combined gain + intensity)
- Knob 5: Tone (100Hz - 1500Hz lowpass)
- Knob 6: Gate threshold

**Technical Notes**:
- Bass boost BEFORE clipping (vintage topology)
- 4x oversampling reduces aliasing
- Asymmetric clipping for harmonic richness
- Gate detects on clean signal, applies to fuzzed

---

### STAGE 5: Autowah AFTER Fuzz

**Active When**:
- `autowah_enabled == true` AND
- `autowah_placement == 1` (Switch 1 MIDDLE)

**Processing**: Identical to Stage 2, but processes post-fuzz signal

**Difference**: Fuzz adds harmonics that autowah can track

---

### STAGE 6: Autowah AFTER Everything

**Active When**:
- `autowah_enabled == true` AND
- `autowah_placement == 2` (Switch 1 DOWN)

**Processing**: Identical to Stage 2/5, but as final effect

**Use Case**: Clean envelope filtering after all other processing

---

### STAGE 6.5: FS2 Gain Compensation

**Active When**:
- (`autowah_enabled == true` OR `octave_enabled == true`) AND
- `fuzz_enabled == false`

**Function**: Compensates for volume loss from FS2 effects when fuzz is bypassed

**Formula**:
```cpp
signal *= 2.0f;  // +6dB makeup gain
```

**Total Compensation**:
- Autowah bandpass: 2.0f (+6dB) built into filter output
- FS2 compensation: 2.0f (+6dB) when fuzz off
- **Total for autowah alone**: 4.0f (+12dB)
- **Total for autowah + fuzz**: 2.0f (+6dB, no FS2 comp)

**Technical Notes**:
- Only applies when fuzz bypassed
- Prevents volume drop from bandpass filter
- Stacks with autowah's built-in 2.0f gain

---

### STAGE 7: Dry/Wet Mix

**Function**: Blends processed signal with clean input

**Formula**:
```cpp
const float mix = knobValues[1];  // 0 to 1
signal = input * (1.0f - mix) + signal * mix;
```

**Control**: Knob 2 (always active)

**Technical Notes**:
- `input` is captured before Stage 1 (pre-gain)
- Equal-power mixing
- CCW = dry, CW = wet

---

### STAGE 8: Output Level

**Function**: Master output volume control

**Formula**:
```cpp
const float level = knobValues[2];  // 0 to 1
signal *= level;
```

**Control**: Knob 3 (always active)

**Technical Notes**:
- Linear scaling
- Final stage before output
- Use to match bypass levels

---

## Signal Routing Examples

### Example 1: Fuzz Only (FS1 ON, FS2 OFF)

```
Input → Stage 1 (Gain) → Stage 4 (Fuzz) → Stage 7 (Mix) → Stage 8 (Level) → Output
```

**Active Stages**: 1, 4, 7, 8

### Example 2: Autowah Before Fuzz (SW1 UP, FS1 ON, FS2 ON)

```
Input → Stage 1 (Gain) → Stage 2 (Autowah) → Stage 3 (Octave*) → Stage 4 (Fuzz) → Stage 7 (Mix) → Stage 8 (Level) → Output
```

*Octave only if enabled via Switch 2

**Active Stages**: 1, 2, 3, 4, 7, 8

### Example 3: Autowah + Octave, No Fuzz (SW1 MIDDLE, FS1 OFF, FS2 ON)

```
Input → Stage 1 (Gain) → Stage 3 (Octave) → Stage 5 (Autowah) → Stage 6.5 (Gain Comp) → Stage 7 (Mix) → Stage 8 (Level) → Output
```

**Active Stages**: 1, 3, 5, 6.5, 7, 8

---

## Gain Staging

### Total Gain Budget (all effects active)

| Stage | Gain | dB | Cumulative |
|-------|------|----|-----------:|
| Input Gain (max) | 2.0x | +6dB | +6dB |
| Bass Boost | 1.8x | +5dB | +11dB |
| Fuzz Gain (max) | 20x | +26dB | +37dB |
| Autowah | 2.0x | +6dB | +43dB |
| FS2 Comp* | 2.0x | +6dB | +49dB |
| Output (max) | 1.0x | 0dB | +49dB |

*Only applies when fuzz bypassed

### Typical Settings

For unity gain with moderate fuzz:
- Input Gain: 12 o'clock (1.25x)
- Drive: 1 o'clock (10x)
- Mix: 75%
- Output: 10 o'clock (0.3x)

---

## CPU Load Analysis

**Measured at 480MHz**:

| Effect | CPU % | Notes |
|--------|------:|-------|
| Base (passthrough) | ~5% | Control processing only |
| + Fuzz | +15% | With 4x oversampling |
| + Autowah | +10% | Envelope + ADSR + SVF |
| + Octave | +35% | Multirate processing |
| All Effects | ~65% | Total measured |

**Headroom**: ~35% available for future additions

---

## Latency Budget

| Component | Samples | ms @ 48kHz |
|-----------|--------:|----------:|
| Audio Block | 256 | 5.33ms |
| Octave Buffer | 6 | 0.13ms |
| Oversampling | 4 | 0.08ms |
| **Total** | **~266** | **~5.54ms** |

---

## Memory Usage

| Region | Used | Available | % |
|--------|-----:|----------:|--:|
| SRAM | ~200KB | 512KB | 39% |
| SDRAM | 0KB | 64MB | 0% |

**Note**: SDRAM unused, available for future features (delays, reverb, etc.)

---

## Optimization Notes

### Current Optimizations
- CPU boosted to 480MHz (from 400MHz)
- Audio block size: 256 samples (balance latency/efficiency)
- Compiler: -Ofast (maximum optimization)
- Octave at 8kHz (6:1 decimation saves CPU)
- Fuzz: 4x oversampling (balance quality/CPU)

### Potential Improvements
- Move to SDRAM for large buffers
- Increase audio block size (lower CPU, higher latency)
- Reduce octave processing rate further
- Remove oversampling from fuzz (more aliasing)

---

## Testing Signal Chain

### Verification Points

1. **After Stage 1**: Should see clean signal with gain applied
2. **After Stage 3**: Octave harmonics visible in spectrum
3. **After Stage 4**: Fuzz distortion and harmonics
4. **After Stage 5/6**: Bandpass filtering evident
5. **After Stage 7**: Dry signal blended back in
6. **After Stage 8**: Final scaled output

### Recommended Test Signals
- **Sine wave (440Hz)**: Clean frequency response
- **Guitar DI**: Real-world performance
- **Pink noise**: Overall frequency balance
- **Impulse**: Transient response

---

## Signal Chain Diagram

```
              ┌──────────────────────────────────────────┐
              │    BUZZBOX OCTA SQUAWKER SIGNAL CHAIN    │
              └──────────────────────────────────────────┘

INPUT
  │
  ├─────────────────────────────────────────────────────┐
  │                                                       │
  ▼                                                       │
┌───────────┐                                           │
│  STAGE 1  │ Input Gain (Knob 1)                       │
│   GAIN    │ 0.5x to 2.0x                              │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 2  │ Autowah BEFORE (if SW1=UP)               │
│  AUTOWAH  │ BPF + ADSR + Envelope                    │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 3  │ Octave (if FS2=ON + SW2)                │
│  OCTAVE   │ Multirate 8kHz processing                │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 4  │ Fuzz (if FS1=ON)                         │
│   FUZZ    │ Bass → Gain → Clip → Gate → Tone        │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 5  │ Autowah AFTER (if SW1=MIDDLE)            │
│  AUTOWAH  │ Same as Stage 2                          │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 6  │ Autowah FINAL (if SW1=DOWN)              │
│  AUTOWAH  │ Same as Stage 2/5                        │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│ STAGE 6.5 │ FS2 Gain Compensation                    │
│ GAIN COMP │ 2x if FS2=ON and Fuzz=OFF               │
└─────┬─────┘                                           │
      │                                                  │
      ├────────────────────────┐                        │
      │                        │                        │
      ▼                        │                        │
┌───────────┐                 │                        │
│  STAGE 7  │◄────────────────┘                        │
│    MIX    │ Blend with dry (Knob 2)                  │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
┌───────────┐                                           │
│  STAGE 8  │ Output Level (Knob 3)                    │
│  OUTPUT   │ 0 to 100%                                │
└─────┬─────┘                                           │
      │                                                  │
      ▼                                                  │
    OUTPUT
```

---

## Key Architectural Decisions

### Why Autowah Has 3 Placement Options
- **Before Fuzz**: Filter shapes input, fuzz responds to filtered signal
- **After Fuzz**: Traditional wah-on-distortion sound
- **After Everything**: Clean envelope filter on full mix

### Why Octave Uses Multirate Processing
- Octave generation is CPU-intensive
- Processing at 8kHz instead of 48kHz saves 6x CPU
- Quality loss minimal for octave effects
- Decimation/interpolation adds <0.2ms latency

### Why Bass Boost Before Clipping
- Vintage fuzz topology (Tone Bender, Fuzz Face)
- Bass frequencies feed harmonics into clipping stages
- Creates richer, fuller distortion character
- Modern approach (boost after) sounds thinner

### Why Gate Detects on Clean Signal
- More accurate envelope tracking
- Not affected by distortion artifacts
- Threshold calibrated for gained but unclipped signal
- Applied to processed output for clean gating

---

## Troubleshooting Signal Path Issues

### Weak Bass in Fuzz
- Check bass boost is before clipping (not after)
- Verify tone control frequency range (100-1500Hz)
- Ensure simple LP filter (not tilt-tone)

### Octave Artifacts/Clicks
- Check buffer alignment (bin_counter management)
- Verify decimation factor matches interpolation
- Ensure buffers cleared when effect off

### Autowah Not Triggering
- Check threshold setting (Knob 5 in autowah mode)
- Verify envelope follower parameters
- Test input signal level

### Volume Jumps When Switching Effects
- Check gain compensation logic
- Verify dry/wet mix scaling
- Test FS2 compensation activation
