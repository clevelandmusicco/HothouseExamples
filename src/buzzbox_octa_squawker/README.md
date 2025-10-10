# Buzzbox Octa Squawker for Hothouse

A versatile fuzz pedal with envelope-controlled autowah filter and octave generation, designed for the Hothouse platform.

## Features

- **Aggressive Vintage Fuzz**: Classic fuzz circuit with bass boost, variable drive, and tone shaping (100-1500Hz)
- **Envelope Autowah**: Dynamic bandpass filter with adjustable speed, threshold, and frequency range
- **Octave Generation**: Independent up/down octave synthesis with mixing control
- **Context-Dependent Controls**: Knobs 4-6 change function based on selected mode
- **Touch-to-Activate**: Parameters hold values after mode switch until knob is moved
- **Flexible Routing**: Three autowah placement options (before/after fuzz/after everything)
- **Automatic Gain Compensation**: Balanced output levels across all effect combinations

## Hardware Requirements

- **Platform**: Hothouse (Daisy Seed based)
- **CPU**: STM32H750 (boosted to 480MHz)
- **RAM**: 512KB SRAM + 64MB SDRAM
- **Flash**: 8MB QSPI Flash (requires bootloader)
- **Connection**: USB cable for programming

## Pre-Built Binary

A pre-compiled binary is included in `src/build/buzzbox_hothouse.bin` for immediate use.

### Flash Using Daisy Web Programmer

1. Connect Hothouse via USB
2. Enter DFU mode: Hold BOOT, press and release RESET, then release BOOT
3. Open [Daisy Web Programmer](https://electro-smith.github.io/Programmer/)
4. Click "Connect" and select your device
5. Click "Choose File" and select `src/build/buzzbox_hothouse.bin`
6. Click "Program"

### Flash Using Command Line

```bash
cd src
make program-dfu
```

## Building from Source

### Dependencies

**Required Libraries:**
- **libDaisy**: Hardware abstraction layer for Daisy Seed
  - Repository: https://github.com/electro-smith/libDaisy
  - Version: Latest stable
- **DaisySP**: Audio DSP library
  - Repository: https://github.com/electro-smith/DaisySP
  - Version: Latest stable with LGPL modules
- **Q DSP Library**: Filter and DSP utilities
  - Repository: https://github.com/cycfi/q
  - License: MIT
- **Hothouse Library**: Hardware interface (hothouse.cpp, hothouse.h)
  - Included in this package

**Build Tools:**
- ARM GCC toolchain (arm-none-eabi-gcc)
- Make
- DFU-util (for flashing)
- Python 3.x (for build scripts)

### Quick Start

**1. Set Up Dependencies**
```bash
# If libDaisy and DaisySP are not already set up:
cd ../..  # Go to HothouseExamples root
git clone https://github.com/electro-smith/libDaisy
git clone https://github.com/electro-smith/DaisySP

# Build libraries
cd libDaisy
make
cd ../DaisySP
make
```

**2. Build Buzzbox Octa Squawker**
```bash
cd src/buzzbox_octa_squawker/src
make clean
make
```

**3. Flash to Hardware**
```bash
# First time only - install bootloader (if needed)
make program-boot

# Flash application
make program-dfu
```

## Build Configurations

### Development Build (SRAM Mode)
- Faster iteration
- No bootloader required
- Limited to 512KB code size

```makefile
# In Makefile, comment out:
# APP_TYPE = BOOT_QSPI
```

### Production Build (QSPI Mode)
- Full 8MB flash available
- Requires bootloader
- Better for complex projects

```makefile
# In Makefile, uncomment:
APP_TYPE = BOOT_QSPI
```

## Documentation

- [Build Instructions](docs/BUILD_INSTRUCTIONS.md) - Detailed compilation guide
- [Controls Reference](docs/CONTROLS_REFERENCE.md) - Complete control mapping
- [Signal Chain](docs/SIGNAL_CHAIN.md) - Audio processing architecture

## Control Overview

### Fixed Controls (Knobs 1-3)
- **Knob 1**: Input Gain (0.5x to 2.0x)
- **Knob 2**: Dry/Wet Mix
- **Knob 3**: Output Level

### Context-Dependent Controls (Knobs 4-6)
Function changes based on **Switch 3** position:
- **UP**: Fuzz controls (drive, tone, gate)
- **MIDDLE**: Autowah controls (speed, threshold, range)
- **DOWN**: Octave controls (up level, down level, mix)

### Toggle Switches
- **Switch 1**: Autowah placement in signal chain
- **Switch 2**: FS2 effect selection (both/autowah/octave)
- **Switch 3**: Knobs 4-6 function mode

### Footswitches
- **FS1**: Fuzz on/off
- **FS2**: Autowah/Octave on/off (per Switch 2)

## Architecture

### Signal Flow
1. Input Gain
2. Autowah (if Switch 1 = UP)
3. Octave Generation (multirate: 8kHz)
4. Fuzz (bass boost → gain → clipping → tone)
5. Autowah (if Switch 1 = MIDDLE)
6. Autowah (if Switch 1 = DOWN)
7. FS2 Gain Compensation
8. Dry/Wet Mix
9. Output Level

### Key Features
- **Multirate Processing**: Octave runs at 8kHz (decimated from 48kHz)
- **Touch-to-Activate**: Prevents parameter jumps on mode switch
- **Gain Compensation**: Automatic level matching for all effect combinations
- **4x Oversampling**: On fuzz stage for reduced aliasing

## Performance Notes

- **CPU Usage**: ~60-70% at 480MHz with all effects active
- **Latency**: ~5.3ms (256 samples @ 48kHz)
- **Memory**: ~200KB SRAM used
- **Optimization**: Compiled with -Ofast for maximum performance

## Troubleshooting

### Build Errors
- Verify ARM toolchain is in PATH
- Check libDaisy and DaisySP are built
- Ensure LIBDAISY_DIR and DAISYSP_DIR paths are correct in Makefile

### Flash Errors
- Verify bootloader is installed (first time only)
- Check device is in DFU mode
- Try different USB cable/port

### Runtime Issues
- Check knob and switch connections
- Verify audio input/output levels
- Test with effects bypassed first

## Attribution

**Original Design**: Chris Brandt (chris@futr.tv)  
**Creation Date**: October 9, 2025

**Inspiration & Components:**
- Keith Bloemer (GuitarML) - Earth Reverbscape inspiration
- Cycfi Q DSP Library (Joel de Guzman) - Octave generation components
- Steve Schulteis - Octave optimization suggestions

**Third-Party Libraries:**
- DaisySP (MIT License)
- libDaisy (MIT License)
- Q DSP Library (MIT License)
- Hothouse Library

## License

MIT License - See LICENSE file for details

## Support

- Issues: [GitHub issues URL]
- Forum: https://forum.electro-smith.com/
- Email: chris@futr.tv
