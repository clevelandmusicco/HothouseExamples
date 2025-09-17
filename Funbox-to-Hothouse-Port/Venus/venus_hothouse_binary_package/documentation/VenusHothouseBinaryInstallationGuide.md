# Venus Hothouse Binary Installation Guide

## Prerequisites
- Hothouse hardware platform with Daisy Seed
- USB-C cable for programming  
- Chrome or Edge browser (Firefox not recommended)

## Installation Steps

### 1. Connect Hardware
Connect your Hothouse to your computer using a USB-C cable.

### 2. Enter DFU Mode
1. Press and hold the **BOOT** button on the Daisy Seed
2. While holding BOOT, press and release the **RESET** button
3. Release the **BOOT** button
4. The device is now in DFU mode

### 3. Open Web Programmer
Navigate to: https://electro-smith.github.io/Programmer/

### 4. Connect to Device
1. Click the **"Connect"** button
2. Select your DFU device from the popup list
3. Click **"Pair"**

### 5. Load Binary
1. Click **"Choose File"**
2. Navigate to and select `venus_hothouse.bin` from the binaries folder
3. The filename should appear next to the button

### 6. Program Device
1. Click **"Program"**
2. Wait for the progress bar to complete
3. You should see a **"Success!"** message

### 7. Exit DFU Mode
Power cycle your Hothouse by disconnecting and reconnecting power.

## Verification
After successful programming:
- LED 1 should be off (bypass mode is active)
- Press Footswitch 1 - LED 1 should toggle with bypass
- Hold Footswitch 2 - LED 2 should light while held (freeze function)

## Troubleshooting

### Device Not Found
- Verify you're in DFU mode (follow step 2 carefully)
- Try a different USB cable
- Try a different USB port
- Ensure you're using Chrome or Edge browser

### Programming Fails
- Re-enter DFU mode and try again
- Clear browser cache and reload the programmer page
- Verify the binary file isn't corrupted

### No Audio After Programming
- Check bypass status (Footswitch 1)
- Verify input/output connections
- Check Mix knob isn't fully counterclockwise

## LED Indicators
- **LED 1 (Red)**: Effect bypass status (Off = Bypassed, On = Active)
- **LED 2 (Red)**: Freeze active (On while Footswitch 2 is pressed)

## Support
For additional help, see the support folder in this distribution.