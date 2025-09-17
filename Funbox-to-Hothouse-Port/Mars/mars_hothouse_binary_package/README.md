# Mars Hothouse Binary Distribution Package v2.0

This package contains the binary distribution of Mars Neural Amplifier Simulator ported to the Hothouse platform.

## Package Contents

- `binaries/` - Mars Hothouse application binary
- `documentation/` - Installation guide with complete setup instructions
- `source_attribution/` - License and attribution information
- `support/` - Web programmer links and resources

## Quick Start

1. Read the installation guide: `documentation/MarsHothouseBinaryInstallationGuide.pdf`
2. Open the Daisy Web Programmer: https://electro-smith.github.io/Programmer/
3. Install the bootloader (one-time setup)
4. Install the Mars Hothouse binary

## What is Mars?

Mars is a neural network-based guitar amplifier and effects processor featuring:
- 3 amp models with clean-to-high-gain progression
- Cabinet simulation with impulse responses
- Musical delay with multiple modes (normal, dotted 8th, triplet)
- Professional tone filtering with high-pass/low-pass modes

## Amp Models (v2.0 Configuration)

The three-way toggle switch selects between:
- **DOWN Position:** Fender Bassman (warm clean tone)
- **MIDDLE Position:** Fender '57 Twin (edge of breakup)
- **UP Position:** Mesa Boogie Mark IIC (high gain lead)

## System Requirements

- Hothouse hardware with DaisySeed
- Chrome or Edge browser (for web programmer)
- USB-C cable

## Support

For detailed installation instructions and troubleshooting, see:
`documentation/MarsHothouseBinaryInstallationGuide.pdf`

For web programmer information and links, see:
`support/web_programmer_links.txt`

## Attribution

**Original Project:** Mars Neural Amplifier Simulator by GuitarML  
**Repository:** https://github.com/GuitarML/FunBox  
**Original Neural Models:** Trained by GuitarML team  
**Hothouse Port:** chris@futr.tv  
**Port Date:** September 13, 2025  
**v2.0 Updates:** Model selection and gain staging improvements  

## Technical Details

- **Neural Network:** RTNeural library with GRU architecture (hidden size = 9)
- **Sample Rate:** 48kHz
- **Processing:** Real-time inference with low-latency audio processing
- **Filter:** Cubic response curve for natural tone control

## License

This software is distributed under the MIT License with Attribution requirement.
See `source_attribution/` folder for complete license and attribution details.

## Version History

- v2.0 (Current) - Revised model selection for better clean-to-distorted progression
  - Replaced problematic Matchless/Klon models with Fender variants
  - Optimized volume compensation for all switch positions
  - Improved gain staging across all positions
  - Enhanced delay processing consistency

- v1.0 (September 13, 2025) - Initial binary release for Hothouse platform
