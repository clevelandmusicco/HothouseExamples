# Earth Hothouse Binary Installation Guide

## Requirements
- Cleveland Music Co. Hothouse pedal
- USB-C cable
- Chrome or Edge web browser (Firefox is not compatible)
- Earth Hothouse binary file (`earth_hothouse.bin`)

## Installation Steps

### 1. Prepare Your Hothouse
- Power off the Hothouse if currently powered
- Connect the Hothouse to your computer using a USB-C cable
- The Hothouse will power on via USB

### 2. Enter DFU Mode
**Critical: Follow this exact sequence:**
1. Press and hold the **BOOT** button
2. While still holding BOOT, press and release the **RESET** button
3. Release the **BOOT** button
4. The Hothouse is now in DFU mode

### 3. Open the Web Programmer
1. Open Chrome or Edge browser
2. Navigate to: https://electro-smith.github.io/Programmer/
3. Click "Connect" button
4. Select "DFU in FS Mode" from the device list
5. Click "Connect" in the dialog

### 4. Install the Binary
1. Click "Choose File" and select `earth_hothouse.bin`
2. Click "Program" to begin installation
3. Wait for "Success!" message (typically 5-10 seconds)
4. The Hothouse will automatically restart

### 5. Verify Installation
- Disconnect the USB cable
- Connect your standard 9V power supply
- LED 1 should be lit (bypass mode)
- Test the footswitches and knobs to verify operation

## Troubleshooting

### Device Not Detected
- Ensure you entered DFU mode correctly (hold BOOT, tap RESET, release BOOT)
- Try a different USB-C cable
- Use Chrome or Edge browser (not Firefox)

### Programming Fails
- Make sure you selected the correct binary file
- Re-enter DFU mode and try again
- Check that no other software is accessing the USB device

### "Last page not writeable" Error
- The Daisy bootloader needs to be installed first
- Contact support for bootloader installation instructions

## Control Overview

### Effect Signal Flow
Input → Octave Processing → Dattorro Reverb → Overdrive (optional) → Mix → Output

### Quick Test
1. Set all knobs to 12 o'clock position
2. Set all toggles to middle position
3. Press Footswitch 1 to engage effect (LED 1 turns off)
4. Play your instrument - you should hear medium reverb
5. Press and hold Footswitch 2 for overdrive swell effect

## Special Features

### Freeze Function
- Set Toggle 3 to UP position
- Hold Footswitch 2 to freeze the reverb tail infinitely
- Release to return to normal decay

### Octave Effects
- Set Toggle 2 to MIDDLE for octave up
- Set Toggle 2 to DOWN for octave up + down
- Use Toggle 3 DOWN position for momentary octave activation

### Overdrive Swell
- Set Toggle 3 to MIDDLE position
- Hold Footswitch 2 for gradual overdrive bloom
- Effect intentionally reduces volume as drive increases for artistic fade

## Support
For additional support, visit:
- Hothouse Examples: https://github.com/clevelandmusicco/HothouseExamples
- Community Forum: https://forum.electro-smith.com/
