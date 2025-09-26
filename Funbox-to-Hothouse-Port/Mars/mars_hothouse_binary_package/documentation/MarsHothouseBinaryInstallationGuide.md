# Mars Hothouse Binary Installation Guide

## Prerequisites
- Hothouse hardware device
- USB-C cable
- Chrome or Edge browser (Firefox is not compatible)
- Internet connection

## Installation Steps

### 1. Connect Your Hothouse
- Connect your Hothouse to your computer using a USB-C cable
- The device should power on (LEDs may light up)

### 2. Open the Daisy Web Programmer
- Navigate to: https://electro-smith.github.io/Programmer/
- The page should load the Web Programmer interface
- If prompted, allow the website to access USB devices

### 3. Enter DFU Mode
**Important: Follow these steps in exact order:**
1. Press and **hold** the BOOT button on your Hothouse
2. While still holding BOOT, press and **release** the RESET button
3. Now release the BOOT button
4. Your Hothouse is now in DFU mode

### 4. Connect to the Device
- Click "Connect" in the Web Programmer
- Select "DFU in FS Mode" from the device list
- Click "Connect" in the dialog

### 5. Load the Binary
- Click "Choose File" 
- Navigate to `binaries/mars_hothouse.bin`
- Select the file and click "Open"

### 6. Program the Device
- Click the "Program" button
- Wait for the progress bar to complete
- You should see "Success!" when done

### 7. Exit DFU Mode
- Press the RESET button once
- Your Hothouse will restart with Mars loaded
- LED 1 should be off (bypass mode)

## Verification
To verify Mars is running correctly:
1. Plug in your guitar
2. Connect audio output
3. Press Footswitch 1 to exit bypass (LED 1 turns on)
4. Play your guitar - you should hear the amp simulation
5. Try different toggle switch positions for different amp models
6. Press Footswitch 2 to enable delay (LED 2 turns on)

## Troubleshooting

### Device Not Recognized
- Ensure you're using Chrome or Edge browser
- Try a different USB cable
- Make sure you entered DFU mode correctly

### Programming Fails
- Re-enter DFU mode and try again
- Ensure the binary file is not corrupted
- Try refreshing the Web Programmer page

### No Sound After Programming
- Check bypass status (Footswitch 1)
- Verify audio connections
- Ensure Knob 3 (Level) is turned up
- Check that DIP switches 1 & 2 are ON (if accessible)

## Version Notes
**v1.1 (September 23, 2025):**
- Footswitch 2 now controls delay enable/disable
- LED 2 indicates delay status
- Delay range optimized to 50ms-1 second
- Output level increased by 25%

**v1.0 (September 13, 2025):**
- Initial Hothouse port from Funbox
- 3 neural amp models
- 3 cabinet impulse responses
- Delay with three modes

## Support
For additional help, visit:
- https://forum.electro-smith.com/
- https://github.com/clevelandmusicco/HothouseExamples
