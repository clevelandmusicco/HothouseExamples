# Mars Hothouse v1.1 - Build Instructions

## Prerequisites

### 1. Install Daisy Toolchain
Follow the official guide for your OS:
- https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment

### 2. Clone Required Libraries

Create a workspace directory:
mkdir ~/daisy
cd ~/daisy

Clone libDaisy:
git clone https://github.com/electro-smith/libDaisy
cd libDaisy
make

Clone DaisySP:
cd ~/daisy
git clone https://github.com/electro-smith/DaisySP
cd DaisySP
make

Note: RTNeural is included in this source package, no need to clone separately.

### 3. Directory Structure
Your directory structure should look like:
~/daisy/
├── libDaisy/
├── DaisySP/
└── mars_hothouse_v1.1_source/
    ├── src/
    │   ├── Makefile
    │   ├── RTNeural/ (included)
    │   └── [source files]
    └── [documentation]

## Building Mars

### 1. Configure Makefile Paths
The Makefile is located in the src/ directory. Edit it to point to your library locations if needed:
# Default paths (adjust if necessary)
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP
# RTNeural is included locally, no path adjustment needed

### 2. Build the Project
cd mars_hothouse_v1.1_source/src/
make clean
make

This will generate build/mars_hothouse.bin

### 3. Install Bootloader (First Time Only)
If you haven't installed the Daisy bootloader:
# Enter DFU mode: Hold BOOT, press RESET, release BOOT
make program-boot

### 4. Program the Binary

Option A: DFU Programming
# Enter DFU mode: Hold BOOT, press RESET, release BOOT
make program-dfu

Option B: Web Programmer
1. Copy build/mars_hothouse.bin to an accessible location
2. Open https://electro-smith.github.io/Programmer/
3. Enter DFU mode on your Hothouse
4. Connect and upload the binary

## Troubleshooting

### Build Errors

"RTNeural not found"
- RTNeural is included in src/RTNeural/
- Check that the directory was copied correctly
- Makefile should reference it locally

"Multiple definition" errors
- Make sure hothouse.cpp is included in CPP_SOURCES
- Don't include .cpp files with #include

"No rule to make target"
- Check all file paths in Makefile
- Ensure you're in the src/ directory when building
- Verify all source files are present

### Programming Errors

"Cannot open DFU device"
- Check USB connection
- Verify DFU mode (hold BOOT, tap RESET, release BOOT)
- Try different USB cable/port

"Last page not writeable"
- Install bootloader first: make program-boot
- Ensure APP_TYPE = BOOT_QSPI in Makefile

### Runtime Issues

No Sound
- Check bypass state (Footswitch 1)
- Verify output level (Knob 3)
- Ensure DIP switches 1 & 2 are ON (if accessible)

Distorted Sound
- Reduce input gain (Knob 1)
- Check mix settings (Knob 2)
- Verify filter isn't at extreme settings

## Memory Configuration

The Makefile includes:
APP_TYPE = BOOT_QSPI

This requires the bootloader but provides more flash memory for the neural network models.

To build for SRAM mode (no bootloader needed, but less memory):
# Comment out for SRAM mode
# APP_TYPE = BOOT_QSPI

## Optimization Settings

The project uses aggressive optimization for performance:
OPT = -Ofast

For debugging, you can change to:
OPT = -Og  # Debug optimization

## CPU Boost

The code enables CPU boost (480MHz) by default:
hw.Init(true);  // true enables boost

This is required for neural network processing to run smoothly.

## Building from Clean State

If you encounter issues, try a complete clean build:
cd mars_hothouse_v1.1_source/src/
make clean
rm -rf build/
make

## Notes
- Always build from the src/ directory
- The Makefile and all source files must be in src/
- RTNeural is included, no external download needed
- Binary output will be in src/build/
