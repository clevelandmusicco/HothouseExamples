# Venus Hothouse - Build Instructions

## Prerequisites

### Required Software
1. **ARM GNU Toolchain** (arm-none-eabi-gcc)
2. **Make** build system
3. **Git** for cloning dependencies
4. **DFU-Util** for programming the Hothouse

### macOS Installation
```bash
brew install armmbed/formulae/arm-none-eabi-gcc
brew install dfu-util
brew install make
```

### Linux (Ubuntu/Debian) Installation
```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi
sudo apt-get install dfu-util
sudo apt-get install build-essential
```

### Windows Installation
- Install ARM GNU Toolchain from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
- Install Make (via MinGW or WSL recommended)
- Install DFU-Util: https://dfu-util.sourceforge.net/

## Project Setup

### 1. Directory Structure
Your development environment should look like this:

```
HothouseExamples/
├── libDaisy/                    # Daisy hardware library
├── DaisySP/                     # Daisy DSP library
└── Funbox-to-Hothouse-Port/
    └── Venus/
        └── venus_hothouse_source/
            ├── venus_hothouse.cpp
            ├── hothouse.cpp
            ├── hothouse.h
            ├── Makefile
            ├── shy_fft.h
            ├── fourier.h
            ├── wave.h
            └── [documentation files]
```

### 2. Clone Required Libraries

If you haven't already, clone libDaisy and DaisySP:

```bash
# Navigate to HothouseExamples root
cd HothouseExamples/

# Clone libDaisy
git clone https://github.com/electro-smith/libDaisy.git
cd libDaisy
git submodule update --init
cd ..

# Clone DaisySP
git clone https://github.com/electro-smith/DaisySP.git
cd DaisySP
git submodule update --init
cd ..
```

### 3. Verify Library Paths

Check that your Makefile has correct paths (already configured):

```makefile
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP
```

These paths assume you're building from `Funbox-to-Hothouse-Port/Venus/venus_hothouse_source/`

## Building Venus

### Clean Build (Recommended)
```bash
# Navigate to the venus_hothouse_source directory
cd Funbox-to-Hothouse-Port/Venus/venus_hothouse_source/

# Clean previous builds
make clean

# Build the project
make
```

### Expected Output
You should see compilation messages and finally:
```
   text	   data	    bss	    dec	    hex	filename
 123456	   1234	  12345	 137035	  2170b	build/venus_hothouse.elf
```

The binary will be created at: `build/venus_hothouse.bin`

### Build Errors?

**"arm-none-eabi-gcc: command not found"**
- ARM toolchain not installed or not in PATH
- Solution: Install toolchain and add to PATH

**"No rule to make target libDaisy"**
- libDaisy not found at specified path
- Solution: Check LIBDAISY_DIR path in Makefile

**"undefined reference to..."**
- Missing source files or libraries
- Solution: Verify all .cpp files are listed in CPP_SOURCES

## Programming the Hothouse

### Method 1: DFU Mode (Recommended)

#### Enter DFU Mode
1. **Connect** Hothouse to computer via USB-C
2. **Press and hold** the BOOT button on the Daisy Seed
3. **While holding BOOT**, press and release the RESET button
4. **Release** the BOOT button
5. The Hothouse is now in DFU mode (LEDs off)

#### Program the Device
```bash
make program-dfu
```

**Alternative**: You can also use the built-in bootloader entry by holding Footswitch 1 for 2 seconds (LEDs will flash alternately 3 times).

### Method 2: Web Programmer

If you prefer a graphical interface:

1. Enter DFU mode (see above)
2. Open [Daisy Web Programmer](https://electro-smith.github.io/Programmer/)
3. Click "Connect" (use Chrome or Edge browser)
4. Select the binary: `build/venus_hothouse.bin`
5. Click "Program"

**Note**: Firefox is not compatible with the Web Programmer.

### Verify Programming
After programming:
1. Press RESET button (or power cycle)
2. LED 1 should light up (bypass mode on)
3. Press Footswitch 1 to toggle bypass
4. Audio should pass through

## Makefile Configuration

### Key Settings

```makefile
TARGET = venus_hothouse           # Output binary name
OPT = -O2                         # Optimization level (-O2 recommended)
CPP_SOURCES = venus_hothouse.cpp hothouse.cpp  # Source files
USE_DAISYSP_LGPL = 1             # Enable DaisySP library
```

### Optimization Levels
- **-O0**: No optimization (useful for debugging, large binary)
- **-O2**: Balanced optimization (recommended for Venus)
- **-Ofast**: Maximum optimization (may cause issues)

Venus uses `-O2` as a good balance between performance and code size.

### Memory Configuration

Venus runs in **SRAM mode** (default) which doesn't require the bootloader. If you need QSPI flash for larger projects:

```makefile
# Uncomment this line for QSPI mode
# APP_TYPE = BOOT_QSPI
```

Then program the bootloader first:
```bash
make program-boot
```

## Troubleshooting

### Build Issues

**"region 'RAM' overflowed"**
- Code too large for available memory
- Solution: Increase optimization level or reduce code size

**"undefined reference to 'hw'"**
- Missing hothouse.cpp in CPP_SOURCES
- Solution: Verify Makefile includes all source files

**"fatal error: shy_fft.h: No such file or directory"**
- DSP header files not in include path
- Solution: Ensure all .h files are in the project directory

### Programming Issues

**"No DFU capable USB device found"**
- Hothouse not in DFU mode
- Solution: Follow DFU entry procedure carefully

**"Cannot open DFU device"**
- Permission issues (Linux)
- Solution: Run with sudo or add udev rules

**"Error during download"**
- USB connection interrupted
- Solution: Use better USB cable, try different port

### Runtime Issues

**No audio output**
- Check bypass LED (should be off for audio processing)
- Verify input connections
- Press Footswitch 1 to toggle bypass

**Crackling or artifacts**
- Sample rate mismatch or buffer issues
- Check that sample rate is set to 32kHz
- Verify audio block size is 256

**Controls not responding**
- ADC not initialized properly
- Verify hw.StartAdc() is called in main()

## Performance Notes

### CPU Usage
Venus is CPU-intensive due to FFT processing:
- **FFT Order**: 12 (4096 points)
- **Sample Rate**: 32 kHz (optimized for FFT)
- **Block Size**: 256 samples
- **Expected Load**: ~85-90% CPU usage

### Memory Usage
- **Flash**: ~100KB compiled code
- **SRAM**: ~50KB for buffers and FFT data
- **Stack**: Standard Daisy configuration

### Real-time Processing
Venus performs real-time spectral processing with 4x overlap. The large FFT size provides excellent frequency resolution but adds latency (~85ms round-trip).

## Development Tips

### Modify Parameters
Edit `venus_hothouse.cpp` to adjust:
- FFT order (line: `const size_t order = 12`)
- Overlap factor (line: `const size_t laps = 4`)
- Sample rate (in main: `hw.SetAudioSampleRate(...)`)

### Add Debug Output
Use the Daisy's print functions:
```cpp
hw.seed.PrintLine("Debug message");
```

### Test Changes
Always test on actual hardware - the spectral processing behaves differently in real-time than in simulation.

## Additional Resources

### Documentation
- **libDaisy Docs**: https://electro-smith.github.io/libDaisy/
- **DaisySP Docs**: https://electro-smith.github.io/DaisySP/
- **Hothouse Examples**: https://github.com/clevelandmusicco/HothouseExamples

### Community
- **Daisy Forum**: https://forum.electro-smith.com/
- **Hothouse Documentation**: See Hothouse repository

### Original Project
- **Funbox Venus**: https://github.com/GuitarML/FunBox/tree/main/software/Venus

## Build Checklist

Before deploying:
- [ ] Clean build completes without errors
- [ ] Binary size within memory limits
- [ ] All DSP files included in project
- [ ] Makefile paths correct
- [ ] DFU programming successful
- [ ] All controls respond correctly
- [ ] Audio processing works in all modes
- [ ] LEDs function properly
- [ ] Bypass toggle works
- [ ] Freeze function works

## Version History
- **v1.0** - Initial build instructions for Hothouse port
