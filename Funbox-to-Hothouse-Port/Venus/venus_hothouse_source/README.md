# Venus Hothouse Source Code Package v1.0

## Overview
Venus is a spectral reverb effect featuring STFT-based processing with shimmer, drift modulation, and detune capabilities. Originally created for the Funbox platform, this is the complete source code for the Hothouse hardware port.

## What is Venus?
Venus is an advanced spectral reverb that processes audio in the frequency domain using Short-Time Fourier Transform (STFT). It offers:

- **Spectral Reverb**: FFT-based reverb with frequency-dependent decay
- **Shimmer Effects**: Octave up, octave down, or both with independent 5th harmonics
- **Drift Modulation**: Automated parameter modulation for evolving textures
- **Detune**: Pitch shifting for chorus-like effects
- **Freeze**: Capture and hold reverb tail
- **Lofi Modes**: Sample rate reduction and filtering for vintage character

## Hardware Requirements
- **Cleveland Music Co. Hothouse** DIY DSP platform
- **Daisy Seed** microcontroller (embedded in Hothouse)
- **USB-C cable** for programming and power

## Quick Start

### Build the Project
```bash
# 1. Ensure you have the Hothouse development environment set up
# 2. Navigate to this directory
cd venus_hothouse_source/

# 3. Clean and build
make clean
make

# 4. Program to Hothouse
make program-dfu
```

See **BUILD_INSTRUCTIONS.md** for detailed setup and compilation instructions.

### Hardware Controls
See **CONTROLS_REFERENCE.md** for complete control mappings.

**Quick Reference:**
- **Knob 1**: Decay (reverb time)
- **Knob 2**: Mix (dry/wet balance)
- **Knob 3**: Damp (high frequency damping)
- **Knob 4**: Shimmer (octave generation amount)
- **Knob 5**: Shimmer Tone (5th harmonic amount)
- **Knob 6**: Detune (pitch shift amount and direction)
- **Toggle 1**: Shimmer mode (octave down/up/both)
- **Toggle 2**: Reverb character (less lofi/normal/more lofi)
- **Toggle 3**: Drift speed (slow/off/fast)
- **Footswitch 1**: Bypass toggle
- **Footswitch 2**: Freeze (momentary, hold to sustain)

## Technical Specifications

### DSP Architecture
- **Sample Rate**: 32 kHz
- **Audio Block Size**: 256 samples
- **FFT Order**: 12 (4096-point FFT)
- **STFT Overlap**: 4x (75% overlap)
- **Processing**: Mono input, stereo output

### Algorithm Details
- **Reverb Engine**: Frequency-domain spectral processing
- **Shimmer**: Octave generation via FFT bin shifting
- **Drift**: 4 independent LFOs modulating parameters
- **Detune**: ±15 cents pitch shifting
- **Lofi Modes**: Sample rate reduction (0.2-0.3x) with low-pass filtering

## File Structure
```
venus_hothouse_source/
├── venus_hothouse.cpp          # Main application code
├── hothouse.cpp                # Hothouse hardware abstraction
├── hothouse.h                  # Hothouse header
├── Makefile                    # Build configuration
├── shy_fft.h                   # FFT implementation
├── fourier.h                   # STFT processing
├── wave.h                      # Window functions
├── README.md                   # This file
├── BUILD_INSTRUCTIONS.md       # Compilation guide
├── CONTROLS_REFERENCE.md       # Hardware control details
└── LICENSE                     # MIT License
```

## Attribution

**Original Project**: Venus Spectral Reverb for Funbox  
**Original Author**: Keith Bloemer (GuitarML)  
**Original Repository**: https://github.com/GuitarML/FunBox/tree/main/software/Venus  
**Original License**: MIT License

**Hothouse Port**: Chris Brandt (chris@futr.tv)  
**Port Date**: September 17, 2025

**DSP Libraries**:
- ShyFFT: FFT implementation for embedded systems
- Custom STFT framework for real-time spectral processing

## Changes from Original Funbox Version

### Hardware Abstraction
- Converted from Funbox (DaisyPetal) to Hothouse platform
- Updated to use Hothouse hardware library (hothouse.cpp/h)
- Changed from `hw.ProcessAnalogControls()` + `hw.ProcessDigitalControls()` to `hw.ProcessAllControls()`
- Updated knob reading to use `hw.GetKnobValue(Hothouse::KNOB_X)`
- Updated toggle switches to use `hw.GetToggleswitchPosition(Hothouse::TOGGLESWITCH_X)`

### Control Mappings
- Corrected toggle switch mapping for Hothouse inverted logic:
  - Physical DOWN = TOGGLESWITCH_DOWN (case 2)
  - Physical MIDDLE = TOGGLESWITCH_MIDDLE (case 1)
  - Physical UP = TOGGLESWITCH_UP (case 0)

### LED Handling
- Updated LED initialization to use `hw.seed.GetPin(Hothouse::LED_X)`
- Both LEDs are red on Hothouse (original Funbox had red/green)

### Audio Configuration
- Maintained original 32kHz sample rate for optimal FFT performance
- Preserved original 256-sample block size

### DSP Processing
- **Preserved all original DSP algorithms exactly**:
  - Spectral reverb calculations unchanged
  - Shimmer formulas preserved
  - Drift modulation identical
  - Detune logic unchanged
- **Parameter curves maintained**:
  - Exponential curve for damp control
  - Linear curves for other parameters
  - Exact scaling ranges preserved

## Development Notes

### Critical Implementation Details

**Parameter Curves**:
The damp parameter uses an exponential curve (squared) while other parameters are linear. This matches the original Funbox implementation and is critical for proper feel.

**Shimmer Calculation**:
The shimmer double/triple/remainder calculations use exact original formulas. Don't "optimize" these - they're carefully tuned for musical results.

**Drift Automation**:
The drift system uses 4 independent oscillators with different frequencies and waveforms. Slower drift uses sine waves, faster drift uses triangle waves.

**Toggle Switch Mapping**:
Hothouse toggle switches have inverted logic compared to Funbox. Physical DOWN position = software case 2.

**Random Phase**:
The reverb uses `rand()*2*PI` directly (not divided by RAND_MAX) to generate random phase. This matches the original and is intentional.

## License
MIT License - See LICENSE file for details.

## Support and Resources

**Hothouse Platform**:
- Repository: https://github.com/clevelandmusicco/HothouseExamples
- Documentation: See Hothouse hardware documentation

**DaisySeed Platform**:
- libDaisy: https://github.com/electro-smith/libDaisy
- DaisySP: https://github.com/electro-smith/DaisySP
- Documentation: https://electro-smith.github.io/libDaisy/

**Original Funbox Project**:
- Repository: https://github.com/GuitarML/FunBox

## Contributing
This is a port of the original Venus project. For issues specific to the Hothouse port, please reference the Hothouse platform. For DSP algorithm questions, refer to the original Funbox project.

## Version History
- **v1.0** (September 17, 2025) - Initial Hothouse source code release
  - Complete port from Funbox to Hothouse platform
  - All DSP algorithms preserved exactly
  - Professional documentation and build system
