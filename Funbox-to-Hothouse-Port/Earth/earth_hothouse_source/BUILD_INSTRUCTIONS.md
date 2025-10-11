# Earth Reverbscape - Build Instructions

Complete instructions for building and programming the Earth Reverbscape Hothouse port from source.

## Prerequisites

### Required Software

**ARM GCC Toolchain:**
- arm-none-eabi-gcc (version 10.3 or later recommended)
- arm-none-eabi-g++
- arm-none-eabi-newlib

**Build Tools:**
- GNU Make
- Git

**Programming Tool (choose one):**
- dfu-util (command-line DFU programming)
- Daisy Web Programmer (browser-based, recommended)

### Recommended Development Environment

- **macOS/Linux**: Native terminal with ARM toolchain
- **Windows**: WSL2 (Windows Subsystem for Linux) recommended
- **All platforms**: VS Code with C/C++ extensions (optional but helpful)

## Installation Instructions

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install ARM toolchain
brew install --cask gcc-arm-embedded

# Install build tools
brew install make dfu-util

# Verify installation
arm-none-eabi-gcc --version
make --version
dfu-util --version
```

### Linux (Ubuntu/Debian)

```bash
# Update package list
sudo apt update

# Install ARM toolchain
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi \
    libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib

# Install build tools
sudo apt install build-essential git dfu-util

# Verify installation
arm-none-eabi-gcc --version
make --version
dfu-util --version
```

### Windows (via WSL2)

```bash
# Open WSL2 terminal and run Linux instructions above

# OR install native Windows ARM toolchain:
# Download from: https://developer.arm.com/downloads/-/gnu-rm
# Install to: C:\Program Files (x86)\GNU Arm Embedded Toolchain
# Add to PATH in System Environment Variables
```

## Project Setup

### 1. Clone Required Repositories

The Earth Reverbscape requires three repositories:

```bash
# Create workspace directory
mkdir -p ~/hothouse_workspace
cd ~/hothouse_workspace

# Clone HothouseExamples (contains Earth source)
git clone https://github.com/clevelandmusicco/HothouseExamples.git

# Clone libDaisy (hardware abstraction library)
git clone https://github.com/electro-smith/libDaisy.git

# Clone DaisySP (DSP library)
git clone https://github.com/electro-smith/DaisySP.git
```

### 2. Directory Structure

After cloning, your workspace should look like this:

```
~/hothouse_workspace/
├── HothouseExamples/
│   └── Funbox-to-Hothouse-Port/
│       └── Earth/
│           └── earth_hothouse_source/  ← Build from here
├── libDaisy/                           ← 3 levels up from source
└── DaisySP/                            ← 3 levels up from source
```

This structure is critical because the Makefile uses relative paths:
```makefile
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP
```

### 3. Build libDaisy

```bash
cd ~/hothouse_workspace/libDaisy
make clean
make
```

**Expected output:** `libdaisy.a` in `build/` directory

### 4. Build DaisySP

```bash
cd ~/hothouse_workspace/DaisySP
make clean
make
```

**Expected output:** `libdaisysp.a` in `build/` directory

## Building Earth Reverbscape

### Navigate to Source Directory

```bash
cd ~/hothouse_workspace/HothouseExamples/Funbox-to-Hothouse-Port/Earth/earth_hothouse_source
```

### Build Commands

**Clean build (recommended):**
```bash
make clean
make
```

**Quick rebuild (after minor changes):**
```bash
make
```

**Verbose output (for troubleshooting):**
```bash
make V=1
```

### Expected Build Output

A successful build produces:

```
Compiling earth_hothouse.cpp...
Compiling hothouse.cpp...
Compiling Dattorro/dsp/filters/OnePoleFilters.cpp...
Compiling Dattorro/dsp/delays/InterpDelay.cpp...
Compiling Dattorro/Dattorro.cpp...
Linking earth_hothouse...
Creating earth_hothouse.bin...

Build complete!
Binary: build/earth_hothouse.bin
Size: ~XXX KB
```

**Output file:** `build/earth_hothouse.bin`

### Makefile Configuration Details

The Earth Makefile includes these key settings:

```makefile
# Project name
TARGET = earth_hothouse

# C++20 required for std::span
CPP_STANDARD = -std=c++20

# Use LGPL version of DaisySP
USE_DAISYSP_LGPL = 1

# Compiler optimization
OPT = -Ofast -fno-strict-aliasing

# All source files including hothouse.cpp
CPP_SOURCES = earth_hothouse.cpp hothouse.cpp
CPP_SOURCES += Dattorro/dsp/filters/OnePoleFilters.cpp
CPP_SOURCES += Dattorro/dsp/delays/InterpDelay.cpp
CPP_SOURCES += Dattorro/Dattorro.cpp

# Include paths for headers
C_INCLUDES += -I.
C_INCLUDES += -Iq/q_lib/include
C_INCLUDES += -Igcem/include
C_INCLUDES += -Iinfra/include
```

## Programming the Hothouse

### Method 1: Web Programmer (Recommended)

1. **Connect Hothouse:** Connect Hothouse to computer via USB-C cable

2. **Enter DFU Mode:**
   - Press and hold the **BOOT** button
   - While holding BOOT, press and release **RESET**
   - Release **BOOT** button
   - The Hothouse is now in DFU (bootloader) mode

3. **Open Web Programmer:**
   - Navigate to: https://electro-smith.github.io/Programmer/
   - Use Chrome or Edge browser (Firefox not supported)

4. **Load Binary:**
   - Click "Connect" and select "DFU in FS Mode" device
   - Click "Choose File" and select `build/earth_hothouse.bin`
   - Click "Program" and wait for completion

5. **Reset:** Press the RESET button to run the new firmware

### Method 2: Command-Line DFU

```bash
# From earth_hothouse_source directory

# Enter DFU mode on Hothouse (see above)

# Program using make
make program-dfu

# OR program directly with dfu-util
dfu-util -a 0 -s 0x08000000:leave -D build/earth_hothouse.bin
```

### Verify Programming

After programming:
- Both LEDs should briefly flash during initialization
- LED 1 (left) indicates bypass state (off = bypassed)
- LED 2 (right) indicates footswitch 2 state (on when held)
- Audio should pass through (in bypass initially)

## Troubleshooting

### Build Errors

**"arm-none-eabi-gcc: command not found"**
- ARM toolchain not installed or not in PATH
- Solution: Install toolchain and add to PATH

**"No rule to make target '../../../libDaisy/core/Makefile'"**
- libDaisy not cloned or wrong directory structure
- Solution: Clone libDaisy three levels up from earth_hothouse_source

**"undefined reference to..." errors**
- Missing source file in CPP_SOURCES
- Solution: Verify hothouse.cpp is included in Makefile

**"error: 'span' is not a member of 'std'"**
- C++ standard not set to C++20
- Solution: Verify `CPP_STANDARD = -std=c++20` in Makefile

**"OnePoleFilters.cpp: No such file or directory"**
- Dattorro directory not present or incomplete
- Solution: Verify Dattorro directory is in earth_hothouse_source

### Programming Errors

**"No DFU capable USB device available"**
- Hothouse not in DFU mode
- Solution: Follow DFU mode entry procedure carefully

**"Error during download"**
- USB connection issue or insufficient permissions
- Solution: Try different USB cable/port, use sudo on Linux

**Device not showing in Web Programmer**
- Wrong browser (Firefox doesn't support WebUSB)
- Solution: Use Chrome or Edge browser

**Binary runs but no audio**
- Verify cables and connections
- Check input/output levels on knobs
- Verify bypass is disengaged (LED 1 should be on)

### Runtime Issues

**Clicking or artifacts in audio**
- Buffer overflow due to excessive CPU usage
- Solution: This shouldn't occur with current settings

**Controls not responding**
- ADC not initialized properly
- Solution: Verify `hw.StartAdc()` is called in main

**LEDs not working**
- LED pins not configured correctly
- Solution: Check LED initialization in main function

## Performance Notes

### CPU Usage

Earth Reverbscape uses approximately:
- **Reverb:** ~35% CPU (Dattorro algorithm)
- **Octave:** ~20% CPU (multirate processing when enabled)
- **Overdrive:** ~5% CPU (dual processors)
- **Total:** ~60% CPU (all effects active)

The Daisy Seed runs at 400MHz by default, providing ample headroom.

### Memory Usage

- **Flash:** ~180KB (program code and constants)
- **SRAM:** ~350KB (reverb buffers, delay lines, etc.)
- **Available:** Ample headroom on Daisy Seed (512KB SRAM)

### Optimization Settings

The build uses `-Ofast` optimization for maximum performance:
- Aggressive compiler optimizations
- Fast math operations
- Loop unrolling and inlining

To reduce binary size (at cost of performance):
```makefile
OPT = -Os  # Optimize for size
```

## Development Tips

### Iterative Development

For faster development cycles:

1. **Make changes** to source files
2. **Quick rebuild:** `make` (no clean needed for minor changes)
3. **Program:** `make program-dfu` or use web programmer
4. **Test** on hardware

### Debugging

**Serial Output:**
```cpp
// Add to earth_hothouse.cpp
hw.seed.PrintLine("Debug message: %f", value);
```

**LED Debugging:**
```cpp
// Blink LED to indicate state
led1.Set(condition ? 1.0f : 0.0f);
led1.Update();
```

### Code Style

Follow the existing code style:
- 2-space indentation
- camelCase for variables
- Clear, descriptive variable names
- Comments for complex DSP sections

## Build Checklist

Before considering a build complete:

- [ ] `make clean && make` completes without errors
- [ ] Binary file created: `build/earth_hothouse.bin`
- [ ] Binary size reasonable (~180KB)
- [ ] No linker warnings
- [ ] Successfully programs to Hothouse
- [ ] All controls respond (6 knobs, 3 toggles, 2 footswitches)
- [ ] Audio passes through cleanly
- [ ] Bypass toggle works (LED 1 indicates state)
- [ ] All three effects functional (reverb, octave, overdrive)
- [ ] No unexpected noise or artifacts

## Additional Resources

**Daisy Documentation:**
- libDaisy: https://electro-smith.github.io/libDaisy/
- Daisy Wiki: https://github.com/electro-smith/DaisyWiki/wiki
- Forum: https://forum.electro-smith.com/

**Hothouse Platform:**
- Examples: https://github.com/clevelandmusicco/HothouseExamples
- Hardware: https://www.clevelandmusicco.com/hothouse/

**ARM Development:**
- GCC ARM: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
- Cortex-M7: https://developer.arm.com/Processors/Cortex-M7

## Support

For build issues or questions:
- Check existing issues: https://github.com/clevelandmusicco/HothouseExamples/issues
- Daisy Forum: https://forum.electro-smith.com/
- Porter: Chris Brandt (chris@futr.tv)