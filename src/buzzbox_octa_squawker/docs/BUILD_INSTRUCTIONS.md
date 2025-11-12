# Buzzbox Octa Squawker Build Instructions

## Prerequisites

### 1. Install ARM Toolchain

**macOS**:
```bash
brew install armmbed/formulae/arm-none-eabi-gcc
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get install gcc-arm-none-eabi
```

**Windows**:
- Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
- Add to PATH

### 2. Verify Installation
```bash
arm-none-eabi-gcc --version
# Should show version 9.x or later
```

### 3. Install DFU Utilities (Optional - for command-line flashing)

**macOS**:
```bash
brew install dfu-util
```

**Linux**:
```bash
sudo apt-get install dfu-util
```

**Windows**:
- Download from http://dfu-util.sourceforge.net/
- Add to PATH

**Note**: You can also use the Daisy Web Programmer instead of dfu-util.

## Library Setup

### 1. Clone Required Libraries

```bash
# Navigate to HothouseExamples root directory
cd /path/to/HothouseExamples

# Clone libDaisy (if not already present)
git clone https://github.com/electro-smith/libDaisy
cd libDaisy
git submodule update --init
make
cd ..

# Clone DaisySP (if not already present)
git clone https://github.com/electro-smith/DaisySP
cd DaisySP
make
cd ..
```

### 2. Verify Library Paths

The Makefile in `src/buzzbox_octa_squawker/src/` is configured for the HothouseExamples structure:

```makefile
# These paths go up three levels from src/buzzbox_octa_squawker/src/
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP
```

**Directory structure should be:**
```
HothouseExamples/
├── libDaisy/
├── DaisySP/
└── src/
    └── buzzbox_octa_squawker/
        └── src/
            └── Makefile (you are here)
```

## Building

### Development Build (SRAM Mode)

For rapid iteration without bootloader:

```bash
cd src/buzzbox_octa_squawker/src

# Edit Makefile - comment out APP_TYPE line:
# APP_TYPE = BOOT_QSPI

make clean
make

# Flash directly (no bootloader needed)
make program
```

**Advantages**:
- Faster development cycle
- No bootloader installation required
- Direct programming via ST-Link

**Limitations**:
- Limited to 512KB code size
- Requires ST-Link programmer

### Production Build (QSPI Mode)

For final release with full 8MB flash:

```bash
cd src/buzzbox_octa_squawker/src

# Edit Makefile - uncomment APP_TYPE line:
APP_TYPE = BOOT_QSPI

make clean
make

# First time only - install bootloader
make program-boot

# Flash application via DFU
make program-dfu
```

## Build Options

### Optimization Levels

In Makefile:
```makefile
# Maximum performance (default)
OPT = -Ofast -fno-strict-aliasing

# Debug build with symbols
OPT = -O0 -g

# Size optimization
OPT = -Os
```

### C++ Standard

```makefile
# Required for std::span in octave processing
CPP_STANDARD = -std=c++20
```

### Block Size

In `buzzbox_hothouse.cpp`:
```cpp
// Larger blocks = more efficient, higher latency
hw.SetAudioBlockSize(256);  // Current: 5.3ms latency

// Smaller blocks = lower latency, less efficient
hw.SetAudioBlockSize(48);   // Alternative: 1ms latency
```

## Flashing Hardware

### Method 1: DFU Mode (Production)

1. **Enter DFU Mode**:
   - Hold BOOT button
   - Press and release RESET
   - Release BOOT

2. **Verify DFU Mode**:
```bash
dfu-util -l
# Should show "Daisy Bootloader"
```

3. **Flash**:
```bash
make program-dfu
```

### Method 2: Daisy Web Programmer (Easiest)

1. **Enter DFU Mode** (same as above)
2. **Open browser** to: https://electro-smith.github.io/Programmer/
3. **Click "Connect"** and select your Hothouse
4. **Choose File**: `build/buzzbox_hothouse.bin`
5. **Click "Program"**

### Method 3: ST-Link (Development)

1. **Connect ST-Link** to SWD pins
2. **Flash**:
```bash
make program
```

## Verification

### 1. Check LEDs
- Both LEDs should be off on boot
- LED 1 lights when FS1 pressed
- LED 2 lights when FS2 pressed

### 2. Test Audio
- Connect input source
- Connect output to amp/headphones
- Set Knob 3 (output level) to 50%
- Should hear clean signal pass-through

### 3. Test Effects
- Press FS1 - should hear fuzz
- Press FS2 - should hear autowah/octave
- Knobs should affect parameters

## Common Build Errors

### Error: "arm-none-eabi-gcc: command not found"
**Solution**: ARM toolchain not in PATH. Reinstall and verify installation.

### Error: "No rule to make target 'hothouse.o'"
**Solution**: Verify hothouse.cpp is in src/ directory and listed in CPP_SOURCES.

### Error: "undefined reference to std::span"
**Solution**: Ensure CPP_STANDARD = -std=c++20 in Makefile.

### Error: "Util/Multirate.h: No such file or directory"
**Solution**: Verify Makefile has `C_INCLUDES += -I..` (not `-I../Util`)

### Error: "region `RAM' overflowed"
**Solution**: Code too large for SRAM. Use QSPI mode (APP_TYPE = BOOT_QSPI).

### Error: Linker fails with Q library errors
**Solution**: Verify q/ library include paths are correct in Makefile:
```makefile
C_INCLUDES += -I../lib/q/q_lib/include
```

## Clean Build

To force complete rebuild:
```bash
make clean
rm -rf build/
make
```

## Performance Profiling

To check CPU usage, add to AudioCallback in buzzbox_hothouse.cpp:
```cpp
static uint32_t last_time = System::GetNow();
if (System::GetNow() - last_time > 1000) {
    float cpu = hw.seed.GetCpuLoad();
    // Print or display cpu usage
    last_time = System::GetNow();
}
```

## Troubleshooting DFU Mode

### Device Not Recognized
1. Try different USB cable
2. Try different USB port
3. Verify BOOT/RESET button sequence
4. Check USB drivers (Windows)

### Programming Fails Partway Through
1. Ensure stable USB connection
2. Don't disconnect during programming
3. Try slower USB 2.0 port instead of USB 3.0

### "No DFU capable USB device available"
1. Verify device is in DFU mode (repeat BOOT/RESET sequence)
2. Check dfu-util installation
3. Try with sudo on Linux/macOS: `sudo make program-dfu`

## Development Workflow

### Typical Development Cycle
```bash
# 1. Make code changes
nano buzzbox_hothouse.cpp

# 2. Build
make

# 3. Flash (if build successful)
make program-dfu

# 4. Test on hardware

# 5. Repeat
```

### Quick Rebuild
```bash
# Only rebuilds changed files
make
```

### Full Rebuild
```bash
# Rebuilds everything
make clean && make
```

## Next Steps

After successful build:
- See [CONTROLS_REFERENCE.md](CONTROLS_REFERENCE.md) for operation
- See [SIGNAL_CHAIN.md](SIGNAL_CHAIN.md) for architecture details
- Start tweaking parameters and experimenting!

## Additional Resources

- **libDaisy Documentation**: https://electro-smith.github.io/libDaisy/
- **DaisySP Documentation**: https://electro-smith.github.io/DaisySP/
- **Daisy Forum**: https://forum.electro-smith.com/
- **ARM GCC Manual**: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
- **Daisy Web Programmer**: https://electro-smith.github.io/Programmer/
