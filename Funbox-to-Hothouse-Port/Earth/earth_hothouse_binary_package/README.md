# Earth Hothouse Binary Distribution Package v1.0

## Quick Start
1. Connect your Hothouse to computer via USB-C
2. Open the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/)
3. Enter DFU mode: Hold BOOT, press and release RESET, then release BOOT
4. Install the Earth Hothouse binary

## What is Earth?
Earth is an advanced reverbscape effect featuring a Dattorro reverb algorithm with spectral octave processing, freeze function, and overdrive swell capabilities. The pedal creates lush, evolving ambient soundscapes with extensive modulation control.

## Hardware Controls

### Knobs
- **Knob 1 (Predelay)**: Reverb pre-delay time (0-1 range)
- **Knob 2 (Mix)**: Dry/wet blend control (0 = fully dry, 1 = fully wet)
- **Knob 3 (Decay)**: Reverb decay time/feedback amount
- **Knob 4 (Mod Depth)**: Reverb tank modulation depth
- **Knob 5 (Mod Speed)**: Reverb tank modulation rate
- **Knob 6 (Damp)**: High frequency damping/filtering

### Toggle Switches
**Toggle 1 - Reverb Size:**
- DOWN (case 2): Small reverb (1x time scale)
- MIDDLE (case 1): Medium reverb (2x time scale)
- UP (case 0): Large reverb (4x time scale)

**Toggle 2 - Effect Mode (Octave):**
- DOWN (case 2): Octave down + up mixed
- MIDDLE (case 1): Octave up only
- UP (case 0): No octave effect

**Toggle 3 - Footswitch 2 Mode:**
- DOWN (case 2): Momentary octave effect
- MIDDLE (case 1): Overdrive swell
- UP (case 0): Freeze reverb

*Note: Toggle positions are inverted - physical DOWN = case 2, UP = case 0*

### Footswitches
- **Footswitch 1**: Toggle bypass (short press) / Enter DFU mode (hold 2 seconds)
- **Footswitch 2**: Momentary effect based on Toggle 3 setting

### LEDs
- **LED 1 (Red)**: Bypass indicator (on = bypassed, off = active)
- **LED 2 (Red)**: Footswitch 2 active indicator

## Attribution
**Original Project:** Earth Reverbscape for Funbox Platform  
**Repository:** https://github.com/GuitarML/Earth  
**Hothouse Port:** Chris Brandt (chris@futr.tv)  
**Port Date:** September 18, 2025

## License
MIT License with Attribution requirement - See source_attribution folder for details

## Technical Notes
- Sample Rate: 48kHz
- Processing includes 6-sample multirate processing for octave generation
- Intentional volume reduction on overdrive swell for bloom/fade effect
- Complex multi-component DSP requiring precise buffer alignment
