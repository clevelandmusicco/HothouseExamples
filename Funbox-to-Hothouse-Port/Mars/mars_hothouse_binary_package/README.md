# Mars Hothouse Binary Distribution Package v1.1

## Quick Start
1. Connect your Hothouse to computer via USB-C
2. Open the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/)
3. Enter DFU mode: Hold BOOT, press and release RESET, then release BOOT
4. Install the Mars Hothouse binary

## What is Mars?
Mars is a neural amp modeler with impulse response cabinet simulation and delay effects. It uses machine learning models trained on real amplifiers to provide authentic tube amp tones with cabinet emulation.

**Version 1.1 Features (September 23, 2025):**
- Added Footswitch 2 as delay enable/disable
- Optimized delay range to 50ms-1 second with linear response
- Increased output level for better volume
- LED 2 shows delay status

## Hardware Controls

### Knobs
- **Knob 1 (Gain)**: Input gain to neural model (0.1 to 2.5)
- **Knob 2 (Mix)**: Dry/wet signal blend
- **Knob 3 (Level)**: Master output volume
- **Knob 4 (Filter)**: Tone control (LP < 50% | HP > 50%)
- **Knob 5 (Delay Time)**: 50ms to 1 second
- **Knob 6 (Delay Feedback)**: Delay repeats amount

### Toggle Switches
**Toggle 1 - Amp Model:**
- UP: Model 1 (Fender '57)
- MIDDLE: Model 2 (Matchless)
- DOWN: Model 3 (Klon)

**Toggle 2 - Cabinet IR:**
- UP: IR 1 (Bright cabinet)
- MIDDLE: IR 2 (Neutral cabinet)  
- DOWN: IR 3 (Dark cabinet)

**Toggle 3 - Delay Mode:**
- UP: Normal delay
- MIDDLE: Dotted eighth
- DOWN: Triplet

### Footswitches
- **Footswitch 1**: Effect bypass
- **Footswitch 2**: Delay enable/disable (v1.1)

### LEDs
- **LED 1 (Red)**: Bypass indicator (on = active, off = bypassed)
- **LED 2 (Red)**: Delay enabled indicator (v1.1)

### DIP Switches (Internal)
- **DIP 1**: Neural model enable (on = enabled)
- **DIP 2**: Impulse response enable (on = enabled)
- **DIP 3**: Not used
- **DIP 4**: Not used

## Attribution
**Original Mars Neural Amp Modeler:** Keith Bloemer (GuitarML)  
**Repository:** https://github.com/GuitarML/Funbox  
**Hothouse Port:** Chris Brandt (chris@futr.tv)  
**Initial Port Date:** September 13, 2025  
**v1.1 Enhancement:** September 23, 2025

## Technical Notes
Mars pushes the Daisy Seed to its limits with neural network processing and impulse response convolution. The models are GRU-based with 9 hidden units, providing a good balance between tone quality and CPU usage.

## License
MIT License with Attribution requirement. See source_attribution folder for details.

## Support
For support and updates, visit the [Hothouse Examples Repository](https://github.com/clevelandmusicco/HothouseExamples)
