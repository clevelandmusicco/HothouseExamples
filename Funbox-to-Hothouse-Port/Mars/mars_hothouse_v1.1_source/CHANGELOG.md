# Mars Hothouse Changelog

## Version 1.1 - September 23, 2025

### New Features
- **Footswitch 2 Delay Control**: Added latching on/off control for delay effect
- **LED 2 Status Indicator**: Shows when delay is active
- **Optimized Delay Range**: Changed from 2-second to 1-second maximum with linear scaling
- **Improved Knob Response**: Full delay time range now usable (50ms at 0% to 1s at 100%)
- **Output Level Boost**: Increased output by 25% for better volume balance

### Bug Fixes
- **Fixed IR Processing**: Removed debug code that was interfering with cabinet simulation
- **Restored Original Amp Models**: Fixed amp model values that were modified during debugging
- **Removed Test Multipliers**: Cleaned up gain staging test code
- **Fixed Signal Chain**: Restored proper Mars processing order

### Technical Changes
- Reduced delay buffer size from 96k to 48k samples (50% memory savings)
- Simplified delay time calculation (removed two-range system)
- Delay processing only active when enabled (CPU savings)
- Original 2-second delay code preserved in comments
- Added delay_bypassed state variable

### Code Improvements
- Better separation of debug vs production code
- Clearer control logic for delay
- Improved LED update structure
- More consistent variable naming

## Version 1.0 - September 13, 2025

### Initial Hothouse Port
- Ported from Funbox Mars Neural Amp Modeler
- Converted hardware interface from DaisyPetal to Hothouse
- Implemented 3 neural amp models (Fender '57, Matchless, Klon)
- Integrated 3 cabinet impulse responses
- Added delay with three modes (normal, dotted eighth, triplet)
- Mapped all Funbox controls to Hothouse equivalents

### Performance Optimizations
- Enabled CPU boost (480MHz)
- Set audio block size to 256 samples
- Compiler optimization set to -Ofast
- Simplified IR processing for embedded platform

### Known Limitations
- Neural network + IR at maximum CPU capacity
- Cannot add additional effects without removing features
- Memory constrained for delay buffer size

## Version 0.9 - September 2025 (Internal)

### Development Version
- Initial working port
- Debug code for testing switch positions
- Experimental gain compensation
- Testing various IR implementations

---

## Upgrade Notes

### From 1.0 to 1.1
- Footswitch 2 function has changed (now controls delay)
- LED 2 now indicates delay status
- Delay time knob has different range (1 second max vs 2 seconds)
- Output level is louder (adjust master volume as needed)

### Building from Source
- No new dependencies required
- Makefile unchanged except for comments
- Binary size similar despite optimizations
