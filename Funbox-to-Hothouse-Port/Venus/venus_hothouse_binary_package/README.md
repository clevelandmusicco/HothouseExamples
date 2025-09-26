# Venus Hothouse Binary Distribution Package v1.0

## Quick Start
1. Connect your Hothouse to computer via USB-C
2. Open the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/)
3. Enter DFU mode: Hold BOOT, press and release RESET, then release BOOT
4. Install the Venus Hothouse binary

## What is Venus?
Venus is a spectral reverb effect that uses FFT processing to create lush, evolving soundscapes. Features include octave shimmer effects, drift modulation, lo-fi modes, and freeze functionality for infinite sustain.

## Hardware Controls

### Knobs
- **Knob 1**: Decay (reverb time, 1-100)
- **Knob 2**: Mix (dry/wet blend)
- **Knob 3**: Damp (frequency damping with exponential curve)
- **Knob 4**: Shimmer (octave effect amount, 0-10%)
- **Knob 5**: Shimmer Tone (5th harmonic blend, 0-30%)
- **Knob 6**: Detune (±15% pitch shift, center = no detune)

### Toggle Switches
**Note:** Physical positions are inverted (DOWN=2, MIDDLE=1, UP=0 in code)

- **Toggle 1**: Shimmer Mode
  - Down: Octave down
  - Middle: Octave up  
  - Up: Octave up + down

- **Toggle 2**: Reverb Character
  - Down: Less lo-fi (with lowpass filter)
  - Middle: Normal (full fidelity)
  - Up: More lo-fi (heavy sample rate reduction)

- **Toggle 3**: Drift Modulation
  - Down: Slow drift (modulates parameters)
  - Middle: No drift
  - Up: Fast drift

### Footswitches
- **Footswitch 1**: Bypass toggle (LED 1 indicates status)
- **Footswitch 2**: Freeze (hold for infinite sustain, LED 2 indicates status)

## Quick Settings

**Basic Reverb:**
- All toggles: MIDDLE
- Decay: 12 o'clock, Mix: 10 o'clock, Damp: 9 o'clock
- Other knobs: Minimum

**Shimmer Pad:**
- Toggle 1: MIDDLE (octave up)
- Knob 4: 2 o'clock (shimmer amount)
- Knob 5: 10 o'clock (add 5ths)

**Ambient Drift:**
- Toggle 3: DOWN (slow drift)
- High Decay, Medium Mix, Low Damp

## Attribution
**Original Project:** Venus Spectral Reverb (Funbox)  
**Repository:** https://github.com/GuitarML/FunBox/tree/main/software/Venus  
**Hothouse Port:** Chris Brandt (chris@futr.tv)  
**Port Date:** September 17, 2025

## License
MIT License with Attribution requirement