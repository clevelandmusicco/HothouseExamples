# Mars Hothouse Binary Installation Guide (Web-Only)

This guide provides complete installation instructions for the Mars Hothouse pedal using only the DaisySeed web uploader - no command-line tools required.

## Overview

**Web Uploader URL:** https://electro-smith.github.io/Programmer/

The Mars Hothouse pedal requires two binaries to be installed:
1. **Daisy Bootloader** (one-time setup)
2. **Mars Hothouse Application** (the actual pedal software)

## Required Files

Your Mars Hothouse package should include:
```
mars_hothouse_package/
├── daisy_bootloader.bin     (official Daisy bootloader)
├── mars_hothouse.bin        (Mars pedal application)
└── installation_guide.pdf   (this document)
```

## Hardware Requirements

- **Hothouse hardware** with DaisySeed installed
- **USB cable** (USB-C to computer)
- **Chrome or Edge browser** (WebUSB support required)

## Installation Process

### Step 1: Install Daisy Bootloader (One-Time Setup)

**This step is required only once per DaisySeed.**

1. **Enter manual DFU mode:**
   - **Hold the BOOT button** on the DaisySeed
   - **While holding BOOT**, press and release the RESET button
   - **Release BOOT button** after pressing RESET
   - LED should be solid or different pattern (not pulsing)

2. **Open web uploader:**
   - Navigate to: https://electro-smith.github.io/Programmer/
   - Click on the **"Bootloader"** tab
   - Click **"Connect"** button
   - Select your DaisySeed from the device list

3. **Upload bootloader:**
   - Select **"v6.3"** from the bootloader version dropdown (recommended for QSPI applications)
   - Click **"Program"** button  
   - Wait for upload completion

4. **Verify bootloader installation:**
   - Disconnect USB cable
   - Reconnect USB cable (without holding any buttons)
   - **LED should now pulse slowly** (breathing pattern)
   - This pulsing indicates bootloader is working correctly

### Step 2: Install Mars Hothouse Application

1. **Enter bootloader mode:**
   
   **Method A: Power Cycle (Quick)**
   - Disconnect and reconnect USB
   - LED will pulse for 2.5 seconds
   - Upload during this window
   
   **Method B: Extended Mode (Easier)**
   - During the pulsing LED period, press **BOOT button once** (don't hold)
   - LED will blink rapidly a few times, then continue pulsing
   - Grace period is now extended indefinitely
   - Upload anytime while LED is pulsing

2. **Upload Mars application:**
   - Open web uploader (if not already open)
   - Ensure device shows as connected
   - Click **"Choose File"** or drag and drop
   - Select **`mars_hothouse.bin`**
   - Click **"Program"** button
   - Wait for upload completion

3. **Verify installation:**
   - Device will automatically restart after upload
   - Mars pedal should boot and be functional
   - LEDs should respond to controls

## Mars Hothouse Controls

Once installed, your Mars pedal features:

### **Knobs:**
- **Knob 1:** Neural network input gain
- **Knob 2:** Mix (dry/wet blend)  
- **Knob 3:** Output level
- **Knob 4:** Filter (low-pass → → high-pass)
- **Knob 5:** Delay time
- **Knob 6:** Delay feedback

### **Toggle Switches:**
- **Toggle 1:** Amp model (Clean / Overdrive / High Gain)
- **Toggle 2:** Cabinet simulation (Bright / Neutral / Dark)
- **Toggle 3:** Delay mode (Normal / Dotted 8th / Triplet)

### **Footswitches:**
- **Footswitch 1:** 
  - **Short press:** Bypass/Engage
  - **Hold 2 seconds:** Enter DFU mode for firmware updates
- **Footswitch 2:** Currently unused (available for future features)

### **LEDs:**
- **LED 1:** Bypass status (on = active, off = bypassed)
- **LED 2:** Currently unused

## Future Firmware Updates

**For future firmware updates, use the built-in DFU mode:**

1. **Enter update mode:**
   - **Hold Footswitch 1 for 2 seconds** while Mars is running
   - Device will flash LEDs and reset into bootloader mode
   - LED will pulse (ready for update)

2. **Upload new firmware:**
   - Use web uploader to install new `mars_hothouse.bin` file
   - No physical access to DaisySeed BOOT button required

**Alternative update method:**
- Use manual DFU mode (BOOT button method) as described in Step 1

## Troubleshooting

### **Bootloader Installation Issues:**

**"Device not found" when connecting:**
- ✅ Use Chrome or Edge browser
- ✅ Use correct DFU sequence: Hold BOOT → Press/Release RESET → Release BOOT
- ✅ Try different USB cable/port

**LED doesn't pulse after bootloader install:**
- ✅ Verify `daisy_bootloader.bin` file is correct
- ✅ Press RESET button to restart into bootloader
- ✅ Check USB connection stability

### **Mars Application Issues:**

**Upload succeeds but pedal doesn't work:**
- ✅ Verify bootloader was installed first
- ✅ Try reinstalling Mars application
- ✅ Check that correct `mars_hothouse.bin` file was used

**SOS LED pattern (3 short, 3 long, 3 short blinks):**
- ✅ Indicates invalid program was uploaded
- ✅ Reinstall bootloader, then Mars application
- ✅ Ensure using correct binary files

**Controls don't respond:**
- ✅ Check all cable connections to Hothouse hardware
- ✅ Verify Mars application installed successfully
- ✅ Try power cycling the device

### **Update Issues:**

**DFU mode via footswitch doesn't work:**
- ✅ Try manual DFU mode using BOOT button method
- ✅ Ensure Mars application is running properly before attempting footswitch DFU
- ✅ Use power cycle method to enter bootloader mode (most reliable)

**Firmware update issues:**
- ✅ Watch for pulsing LED indicating bootloader grace period
- ✅ Press BOOT button during grace period to extend it indefinitely
- ✅ Ensure USB connection is stable during upload

## Important Notes

- **Bootloader installation is required only once** per DaisySeed
- **Mars application can be updated easily** using built-in DFU mode or manual BOOT button method
- **Always use the provided binary files** - other versions may not work
- **Web uploader requires Chrome or Edge browser** for WebUSB support
- **Keep both binary files** for future reference or troubleshooting

## Installation Summary

1. ✅ **Install bootloader** (one-time): Manual DFU mode → `daisy_bootloader.bin`
2. ✅ **Install Mars app**: Bootloader mode → `mars_hothouse.bin`  
3. ✅ **Future updates**: Footswitch 1 hold (2s) OR manual DFU → new `mars_hothouse.bin`

Your Mars Hothouse pedal is now ready to use!