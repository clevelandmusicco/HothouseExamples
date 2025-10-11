# Earth Reverbscape - Hothouse Source Code

Complete source code for the Earth Reverbscape effect pedal, ported to the Hothouse platform from the original Funbox implementation by Keith Bloemer (GuitarML).

## Quick Start

**Prerequisites:**
- ARM GCC toolchain installed
- libDaisy and DaisySP libraries cloned
- Make and dfu-util installed

**Build:**
```bash
cd earth_hothouse_source
make clean
make
```

**Program:**
```bash
# Enter DFU mode: Hold BOOT, press/release RESET, release BOOT
make program-dfu
# Or use the web programmer: https://electro-smith.github.io/Programmer/
```

## What is Earth Reverbscape?

Earth Reverbscape is a sophisticated multi-effect pedal combining three distinct effects:

**Reverb Engine:** Based on the Mooer A7 ambience reverb algorithm (Dattorro plate reverb), featuring adjustable pre-delay, decay, modulation, and damping controls.

**Octave Generator:** Multirate octave generation providing sub-octave, octave down, and octave up with intelligent pitch tracking and EQ compensation.

**Overdrive:** Smooth drive/gain stage with automatic volume compensation, perfect for adding grit to the reverb tail or creating ambient distortion textures.

The effects can be routed in various configurations, with momentary control options for freeze, overdrive boost, and effect toggling.

## Hardware Requirements

- Hothouse DIY DSP Platform (Daisy Seed based)
- 6 potentiometers (knobs)
- 3 SPDT toggle switches (ON-OFF-ON)
- 2 momentary footswitches
- 2 LEDs (both red on Hothouse)

## Control Reference

### Knobs
- **KNOB 1**: Reverb Pre-delay (0-100ms)
- **KNOB 2**: Dry/Wet Mix (full dry to full wet)
- **KNOB 3**: Reverb Decay (short to infinite)
- **KNOB 4**: Modulation Depth (subtle to deep chorus)
- **KNOB 5**: Modulation Speed (slow to fast)
- **KNOB 6**: Damping (dark to bright reverb character)

### Toggle Switches

**TOGGLE 1 - Reverb Time Scale:**
- UP (0): Normal (1.0x)
- MIDDLE (1): Extended (2.0x) 
- DOWN (2): Long (4.0x)

**TOGGLE 2 - Effect Mode:**
- UP (0): Reverb only
- MIDDLE (1): Reverb + Octave up
- DOWN (2): Reverb + Octave up + Octave down + Sub-octave

**TOGGLE 3 - Footswitch 2 Function:**
- UP (0): Freeze (infinite reverb)
- MIDDLE (1): Overdrive boost
- DOWN (2): Momentary effect toggle

### Footswitches
- **FOOTSWITCH 1**: True bypass toggle (LED 1: red when active)
- **FOOTSWITCH 2**: Momentary function based on TOGGLE 3 setting (LED 2: red when held)

**Special Function:** Hold FOOTSWITCH 1 for 2 seconds to enter bootloader mode (LEDs flash alternately 3 times).

## Technical Specifications

**DSP Architecture:**
- Dattorro plate reverb with input diffusion and tank modulation
- Multirate octave generation (6x decimation for pitch tracking)
- Dual overdrive processors with volume compensation
- High/low shelf EQ for octave mixing
- Expression pedal support via MIDI

**Audio Processing:**
- Sample Rate: 48 kHz
- Block Size: 48 samples
- Bit Depth: 24-bit
- Latency: ~1ms base latency
- Processing: 32-bit floating point

**Effects Chain:**
1. Input buffering for octave processing
2. Multirate octave generation (when enabled)
3. EQ compensation for octave signals
4. Reverb processing (Dattorro algorithm)
5. Optional overdrive stage
6. Dry/wet mixing with freeze reduction

## File Structure

```
earth_hothouse_source/
├── earth_hothouse.cpp          # Main application code
├── hothouse.cpp                # Hothouse hardware abstraction
├── hothouse.h                  # Hothouse hardware interface
├── Makefile                    # Build configuration
├── expressionHandler.h         # Expression pedal MIDI handler
├── Dattorro/                   # Reverb algorithm implementation
│   ├── Dattorro.hpp
│   ├── Dattorro.cpp
│   └── dsp/                    # DSP components
├── Util/                       # Utility DSP modules
│   ├── Multirate.h             # Decimation/interpolation
│   └── OctaveGenerator.h       # Pitch shifting
├── q/                          # Q DSP library (filters, etc.)
├── gcem/                       # Compile-time math functions
├── infra/                      # Infrastructure utilities
├── README.md                   # This file
├── BUILD_INSTRUCTIONS.md       # Detailed build guide
├── CONTROLS_REFERENCE.md       # Complete control documentation
└── LICENSE                     # MIT license
```

## Attribution

**Original Project:** Earth Reverbscape for Funbox  
**Original Author:** Keith Bloemer (GuitarML)  
**Original Repository:** https://github.com/GuitarML/FunBox/tree/main/software/Earth  
**Hothouse Port:** Chris Brandt (chris@futr.tv)  
**Port Date:** September 5, 2025  
**License:** MIT License (see LICENSE file)

## Changes from Original Funbox Version

The Hothouse port preserves the complete original DSP implementation while adapting the hardware interface:

**Hardware Adaptations:**
- Hothouse hardware abstraction layer (GPIO, ADC, controls)
- Toggle switch position mapping (inverted from Funbox)
- LED control adapted for Hothouse dual red LEDs
- Bootloader reset via long footswitch press

**Preserved Original Features:**
- Complete Dattorro reverb algorithm (no modifications)
- Multirate octave generation and processing
- Overdrive volume compensation formula
- Control parameter curves and smoothing
- Expression pedal MIDI implementation
- All effect routing and mixing logic

## Development Notes

**Critical Implementation Details:**

**Multirate Processing:** The octave generator processes every 6th sample at a reduced rate to enable accurate pitch tracking. Buffer management is critical - the code maintains separate input (`buff`) and output (`buff_out`) buffers synchronized with the bin counter.

**Volume Compensation:** The overdrive stage uses the original artistic formula for volume compensation: `1.0f - (current_ODswell * current_ODswell * 2.8f - 0.1296f)`. This creates a bloom/fade effect that is intentional.

**Freeze Implementation:** When freeze is active (TOGGLE 3 UP + FOOTSWITCH 2 held), the reverb decay is ramped to 1.0 (infinite), and the wet output is reduced by 40% to prevent excessive build-up while maintaining the frozen tail.

**Toggle Switch Mapping:** Hothouse toggles are inverted from Funbox:
- Physical UP → case 0 (TOGGLESWITCH_UP)
- Physical MIDDLE → case 1 (TOGGLESWITCH_MIDDLE)
- Physical DOWN → case 2 (TOGGLESWITCH_DOWN)

## Building and Programming

See `BUILD_INSTRUCTIONS.md` for complete setup, build, and programming instructions.

## Detailed Control Documentation

See `CONTROLS_REFERENCE.md` for comprehensive control descriptions, parameter ranges, and usage examples.

## License and Support

This project is licensed under the MIT License - see the `LICENSE` file for details.

**Support Resources:**
- Hothouse Examples: https://github.com/clevelandmusicco/HothouseExamples
- Daisy Forum: https://forum.electro-smith.com/
- libDaisy Documentation: https://electro-smith.github.io/libDaisy/

## Version History

**v1.0 (September 5, 2025)**
- Initial Hothouse port from Funbox
- Complete DSP preservation
- Hardware abstraction via Hothouse library
- Professional source distribution package