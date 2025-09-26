# Attribution for Mars Hothouse Port

## Original Project
**Project Name:** Mars Neural Amp Modeler  
**Creator:** Keith Bloemer (GuitarML)  
**Original Platform:** Funbox (Daisy Seed)  
**Repository:** https://github.com/GuitarML/Funbox  
**License:** MIT License  

## Hothouse Port
**Porter:** Chris Brandt  
**Contact:** chris@futr.tv  
**Initial Port Date:** September 13, 2025  
**Version 1.1 Update:** September 23, 2025  
**Target Platform:** Hothouse (Daisy Seed)  

## Changes Made for Hothouse Port

### Hardware Interface Changes
- Replaced Funbox hardware abstraction with Hothouse library
- Mapped Funbox controls to Hothouse equivalents
- Updated LED initialization for Hothouse pinout
- Converted from DaisyPetal to Hothouse class

### Control Modifications (v1.1)
- Added Footswitch 2 as delay enable/disable control
- Modified delay range from 2 seconds to 1 second maximum
- Implemented linear delay time scaling (50ms to 1 second)
- Added LED 2 as delay status indicator
- Increased output level by 25% (0.4 to 0.5 multiplier)

### Performance Optimizations
- Enabled CPU boost (480MHz) for neural network processing
- Set audio block size to 256 samples
- Simplified impulse response processing for Daisy Seed
- Optimized delay buffer size for memory efficiency

## Third-Party Libraries and Components

### RTNeural
- **Purpose:** Neural network inference for amp modeling
- **License:** BSD 3-Clause
- **Repository:** https://github.com/jatinchowdhury18/RTNeural

### DaisySP
- **Purpose:** DSP library for audio processing
- **License:** MIT License
- **Repository:** https://github.com/electro-smith/DaisySP

### libDaisy
- **Purpose:** Hardware abstraction layer
- **License:** MIT License
- **Repository:** https://github.com/electro-smith/libDaisy

### Eigen
- **Purpose:** Linear algebra for neural network operations
- **License:** MPL2
- **Repository:** https://gitlab.com/libeigen/eigen

### Neural Network Models
- **Source:** GuitarML model collection
- **Training:** Based on real amplifier captures
- **Models Included:**
  - Fender '57 (Model 1)
  - Matchless (Model 2)
  - Klon (Model 3)
  - Additional models in collection

### Impulse Response Data
- **Purpose:** Cabinet simulation
- **Format:** Simplified for embedded processing
- **Processing:** Adapted from NeuralAmpModeler

## Acknowledgments
Special thanks to:
- Keith Bloemer for creating the original Mars neural amp modeler
- The GuitarML community for amp modeling research
- Cleveland Music Co. for the Hothouse platform
- Electro-Smith for the Daisy ecosystem

## License Notice
This port maintains the original MIT License with attribution requirements. Users must retain this attribution file and credit both the original creator and the porter when distributing or modifying this software.

For the complete license text, see funbox_mars_original_license.txt
