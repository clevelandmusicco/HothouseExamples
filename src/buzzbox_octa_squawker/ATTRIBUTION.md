# Attribution for Buzzbox Octa Squawker

## Original Creation

**Project Name:** Buzzbox Octa Squawker  
**Platform:** Hothouse (Daisy Seed)  
**Creator:** Chris Brandt  
**Contact:** chris@futr.tv  
**Creation Date:** October 8, 2025  
**Repository:** https://github.com/clevelandmusicco/HothouseExamples  
**License:** MIT License  

## Project Description

Buzzbox Octa Squawker is an original multi-effect pedal combining vintage fuzz, envelope-controlled autowah, and octave generation. Built specifically for the Hothouse platform, it features context-dependent controls, flexible signal routing, and multirate octave processing.

## Key Contributors

### Keith Bloemer (GuitarML)
- **Contribution**: Inspiration from Earth Reverbscape project and optimization suggestions
- **Repository**: https://github.com/GuitarML/Earth
- **Impact**: Influenced overall architecture and provided valuable performance optimization insights

### Steve Schulteis
- **Contribution**: Created OctaveGenerator and help optimizing Poly Octave Code
- **Repository**: https://github.com/schult/terrarium-poly-octave
- **Impact**: Core octave generation functionality, critical optimization suggestions for multirate processing

### Joel de Guzman (Cycfi Research)
- **Contribution**: Cycfi Q DSP Library
- **Repository**: https://github.com/cycfi/q
- **License**: MIT License
- **Impact**: Multirate processing components (decimation/interpolation), biquad filters, EQ processing

## Third-Party Libraries

### DaisySP
- **Copyright**: (c) 2020 Electrosmith, Corp.
- **License**: MIT License
- **Repository**: https://github.com/electro-smith/DaisySP
- **Usage**: Core DSP components including:
  - Envelope follower
  - ADSR envelope generator
  - State variable filter (SVF)
  - Tone filters
  - Overdrive (unused in current implementation)
  - Basic oscillators and effects

### libDaisy
- **Copyright**: (c) 2019 Electrosmith, Corp.
- **License**: MIT License
- **Repository**: https://github.com/electro-smith/libDaisy
- **Usage**: Hardware abstraction layer for Daisy Seed platform
  - GPIO management
  - Audio I/O
  - System initialization
  - USB/MIDI support

### Hothouse Library
- **Copyright**: Cleveland Music Co.
- **Repository**: https://github.com/clevelandmusicco/HothouseExamples
- **Usage**: Hardware interface layer for Hothouse platform
  - Control processing (knobs, switches, footswitches)
  - LED management
  - Platform-specific configurations

### Cycfi Q DSP Library
- **Copyright**: (c) 2014-2021 Joel de Guzman
- **License**: MIT License
- **Repository**: https://github.com/cycfi/q
- **Usage**:
  - Multirate processing (decimation/interpolation)
  - Biquad filter implementations
  - High-shelf and low-shelf EQ for octave processing
  - DSP utility functions

## Technical Acknowledgments

### Multirate Processing
The octave generation uses 6:1 decimation to process at 8kHz instead of 48kHz, significantly reducing CPU load while maintaining audio quality. This approach was inspired by Earth Reverbscape's octave implementation and optimized with guidance from Steve Schulteis.

### Fuzz Circuit Topology
The fuzz processing follows vintage circuit topology (inspired by Tone Bender Mk III and similar designs) with bass boost before clipping, which is critical for authentic vintage fuzz character.

### Envelope Following and Autowah
The autowah implementation uses envelope detection coupled with ADSR for natural, musical filter sweeps. The bandpass filter approach with gain compensation ensures consistent volume across effect settings.

### Touch-to-Activate Control System
The context-dependent control system with touch-to-activate prevents parameter jumps when switching between fuzz, autowah, and octave control modes, enabling smooth workflow during performance.

## Build System

### ARM GCC Toolchain
- **Copyright**: Free Software Foundation, Inc.
- **License**: GPL v3 (compiler), libraries vary
- **Usage**: Cross-compilation for ARM Cortex-M7 target

### GNU Make
- **Copyright**: Free Software Foundation, Inc.
- **License**: GPL v3
- **Usage**: Build automation

## Platform Acknowledgments

### Hothouse Platform
- **Developer**: Cleveland Music Co.
- **Platform**: Custom Daisy Seed-based guitar pedal platform
- **Features**: 6 knobs, 3 toggle switches, 2 footswitches, 2 LEDs

### Daisy Seed
- **Developer**: Electrosmith, Corp.
- **Platform**: ARM Cortex-M7 based embedded audio platform
- **Processor**: STM32H750 @ 480MHz (overclocked from 400MHz)

## Design Philosophy

Buzzbox Octa Squawker was designed with the following principles:

1. **Vintage Character**: Authentic fuzz circuit topology with bass boost before clipping
2. **Flexibility**: Three autowah placement options for different sonic characteristics
3. **Performance**: Context-dependent controls maximize available interface
4. **Efficiency**: Multirate octave processing optimizes CPU usage
5. **Musicality**: Touch-to-activate prevents parameter jumps during mode switching

## Community and Support

### Daisy Community
- Forum: https://forum.electro-smith.com/
- Active community providing support and sharing knowledge

### Open Source Contribution
This project builds upon and contributes back to the open-source embedded audio community. All original code is released under MIT License to enable further innovation.

## Version History

### v1.0 (October 8, 2025)
- Initial release
- Full feature set: fuzz, autowah, octave generation
- Context-dependent controls with touch-to-activate
- Three autowah placement options
- Multirate octave processing
- Automatic gain compensation

## Contact and Support

**Creator**: Chris Brandt  
**Email**: chris@futr.tv  
**Issues**: https://github.com/clevelandmusicco/HothouseExamples/issues  
**Forum**: https://forum.electro-smith.com/

## License

This project is licensed under the MIT License. See the LICENSE file for full details.

Copyright (c) 2025 Chris Brandt

## Special Thanks

- Keith Bloemer for inspiration and optimization guidance
- Steve Schulteis for octave generation code and optimization help
- Joel de Guzman for the excellent Q DSP library
- Cleveland Music Co. for the Hothouse platform
- Electrosmith for the Daisy ecosystem
- The entire Daisy community for ongoing support and inspiration
