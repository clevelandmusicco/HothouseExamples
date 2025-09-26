# Attribution for Earth Hothouse Port

## Original Project
**Project Name:** Earth Reverbscape  
**Platform:** Funbox (Daisy Seed)  
**Original Author:** Keith Bloemer (GuitarML)  
**Original Repository:** https://github.com/GuitarML/Earth  
**License:** MIT License  

## Hothouse Port
**Porter:** Chris Brandt  
**Contact:** chris@futr.tv  
**Port Date:** September 18, 2025  
**Port Repository:** https://github.com/clevelandmusicco/HothouseExamples  

## Modifications for Hothouse

### Hardware Interface Changes
- Replaced Funbox hardware abstraction with Hothouse library
- Converted from separate analog/digital control processing to unified `hw.ProcessAllControls()`
- Updated knob reading from `Parameter::Process()` to `hw.GetKnobValue()`
- Modified toggle switch reading to use `hw.GetToggleswitchPosition()`
- Adapted LED control for Hothouse pin mapping

### Audio Processing Adaptations
- Maintained original Dattorro reverb algorithm implementation
- Preserved octave generation with 6-sample multirate processing
- Kept intentional volume reduction on overdrive swell (artistic bloom/fade effect)
- Fixed buffer alignment issues for multirate processing integration
- Removed expression pedal physical input (no hardware support on Hothouse)

### Memory and Performance
- Configured for QSPI boot mode due to complexity
- Optimized control processing to prevent audio dropouts
- Moved bootloader check from audio callback to main loop

## Third-Party Components

### Dattorro Reverb Algorithm
- Implementation based on Jon Dattorro's reverb design
- Original paper: "Effect Design Part 1: Reverberator and Other Filters" (1997)
- License: Check original Earth repository for specific terms

### DaisySP Library
- Copyright (c) 2020 Electrosmith
- MIT License
- Used for core DSP components (ReverbSc, Overdrive, basic filters)

### Cycfi Q DSP Library
- Used for biquad filters and EQ processing
- License: MIT License
- Copyright (c) 2014-2021 Joel de Guzman

### Multirate Processing Components
- Custom implementation for octave generation
- Based on standard decimation/interpolation techniques
- Included from original Earth project

## Acknowledgments
- Thanks to Keith Bloemer for the original Earth implementation
- Cleveland Music Co. for the Hothouse platform
- Electrosmith for the Daisy ecosystem
- The Daisy community for ongoing support

## Important Notes
- This port maintains full compatibility with the original Earth functionality
- The "backwards" volume compensation in overdrive is intentional, not a bug
- Buffer index management is critical for proper octave processing
- Toggle switch positions are inverted on Hothouse (physical DOWN = case 2)
