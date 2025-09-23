# Mars Hothouse v1.1 - Dependencies

## Required Libraries

### libDaisy
- **Version**: Latest (as of September 2025)
- **Purpose**: Hardware abstraction layer for Daisy Seed
- **Repository**: https://github.com/electro-smith/libDaisy
- **License**: MIT

### DaisySP
- **Version**: Latest (as of September 2025)
- **Purpose**: DSP library for audio processing
- **Repository**: https://github.com/electro-smith/DaisySP
- **License**: MIT

### RTNeural
- **Version**: Compatible with Daisy Seed
- **Purpose**: Neural network inference engine
- **Repository**: https://github.com/jatinchowdhury18/RTNeural
- **License**: BSD 3-Clause
- **Note**: Must be cloned with --recursive flag for submodules

### Eigen (via RTNeural)
- **Version**: Included with RTNeural
- **Purpose**: Linear algebra operations
- **Repository**: https://gitlab.com/libeigen/eigen
- **License**: MPL2

## Toolchain Requirements

### ARM GCC Compiler
- **Version**: arm-none-eabi-gcc 10.3 or later
- **Purpose**: Cross-compilation for ARM Cortex-M7
- **Installation**: Part of Daisy Toolchain

### Make
- **Version**: GNU Make 3.81 or later
- **Purpose**: Build automation
- **Installation**: Usually pre-installed on Unix systems

### dfu-util (optional)
- **Version**: 0.9 or later
- **Purpose**: Device Firmware Update programming
- **Installation**: Part of Daisy Toolchain

## Included Components

### Neural Network Models
- **Source**: GuitarML training scripts
- **Format**: GRU with 9 hidden units
- **Models**: Fender '57, Matchless, Klon
- **File**: all_model_data_gru9_4count.h

### Impulse Response Data
- **Source**: Cabinet measurements
- **Format**: Simplified for embedded use
- **IRs**: 3 cabinet simulations
- **File**: ir_data.h

### Hothouse Library
- **Version**: 1.0
- **Purpose**: Hardware interface for Hothouse platform
- **Files**: hothouse.cpp, hothouse.h
- **Author**: Cleveland Music Co.

## Directory Structure Expected

Project assumes this relative structure:
- ../../libDaisy/
- ../../DaisySP/
- ../../RTNeural/

Adjust paths in Makefile if your structure differs.

## Version Compatibility Notes

- Daisy Seed Rev 7 or later recommended for stability
- RTNeural version must support Daisy's limited memory
- Eigen headers must be accessible to RTNeural

## Build Environment

Tested on:
- macOS 12+ with Homebrew toolchain
- Ubuntu 20.04+ with apt toolchain
- Windows 10+ with MSYS2

## Memory Requirements

- Flash: ~200KB for binary
- RAM: Uses most available RAM for processing
- SDRAM: 48KB for delay buffer
- QSPI: Required for bootloader mode
