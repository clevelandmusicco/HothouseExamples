# Mars Hothouse v1.1 - Source Release

## Overview
Mars is a neural amp modeler with impulse response cabinet simulation for the Hothouse platform. This source release includes the complete v1.1 implementation with enhanced delay controls.

## Version 1.1 Features
- Neural network amp modeling (3 models included)
- Impulse response cabinet simulation (3 IRs included)
- Delay with normal/dotted eighth/triplet modes
- **NEW**: Footswitch 2 delay enable/disable
- **NEW**: Optimized 50ms-1s delay range
- **NEW**: LED 2 delay status indicator
- **FIED**: IR processing now works correctly
- **FIXED**: Restored original amp model values

## Building from Source

### Prerequisites
- Daisy Toolchain installed
- DaisySP library
- libDaisy library
- RTNeural (included headers)
- ARM GCC compiler

### Quick Build
cd src/
make clean
make

### For DFU Programming
bash
make program-dfu

### Hardware Requirements
- Hothouse pedal platform
- Daisy Seed (Rev 7 or later recommended)
- USB-C cable for programming

## Project Structure
- `src/` - All source code and build files
- `src/Makefile` - Build configuration  
- `src/ImpulseResponse/` - IR processing code
- `src/RTNeural/` - Neural network library (included)
- `BUILD_INSTRUCTIONS.md` - Detailed build guide
- `CHANGELOG.md` - Version history
- `DEPENDENCIES.md` - Required libraries

### Configuration
#### Performance Settings
The code includes several performance optimizations:
- CPU boost enabled (480MHz)
- Audio block size: 256 samples
- Compiler optimization: -Ofast

#### Memory Configuration
- Uses QSPI boot mode
- Delay buffer in SDRAM
- 48k samples (1 second) delay buffer

### Controls
- Knob 1: Input gain (0.1-2.5)
- Knob 2: Dry/wet mix
- Knob 3: Output level
- Knob 4: Tone (LP/HP filter)
- Knob 5: Delay time (50ms-1s)
- Knob 6: Delay feedback
- Toggle 1: Amp model select
- Toggle 2: Cabinet IR select
- Toggle 3: Delay mode
- Footswitch 1: Bypass (long press DFU)
- Footswitch 2: Delay on/off

### License
MIT License - See LICENSE file for details

### Attribution
- Original Mars by Keith Bloemer (GuitarML)
- Hothouse port by Chris Brandt
- v1.1 enhancements September 23, 2025

### Support
- https://github.com/clevelandmusicco/HothouseExamples
- https://github.com/GuitarML/Funbox

