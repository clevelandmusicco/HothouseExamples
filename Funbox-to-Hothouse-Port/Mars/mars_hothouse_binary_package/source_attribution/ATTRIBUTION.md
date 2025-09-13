# Mars Hothouse - Attribution and Credits

## Original Project Attribution

**Mars Hothouse** is a port of the **Mars Neural Amplifier Simulator** originally developed for the Funbox platform.

### Original Mars Project:
* **Project Name:** Mars Neural Amplifier Simulator  
* **Original Platform:** GuitarML Funbox
* **Original Repository:** https://github.com/GuitarML/FunBox
* **Original Author:** GuitarML Team (Keith Bloemer)
* **Original License:** MIT License with Attribution requirement

### Project Description:
The original Mars project is a neural network-based guitar amplifier and effects processor that uses trained models to simulate real amplifiers including the Matchless SC30, Klon Centaur, and Mesa Boogie IIB. The original implementation included impulse response cabinet simulation, musical delay with multiple modes, and advanced wet/dry signal processing.

## Hothouse Port Attribution

### Porting Work:
* **Target Platform:** Cleveland Music Co. Hothouse
* **Architecture Conversion:** Funbox → Hothouse hardware abstraction
* **Porting Date:** September 13, 2025
* **Porter:** chris@futr.tv

### Porting Implementation:
The Hothouse port preserves all original Mars functionality while adapting to the Hothouse hardware architecture:

* **Neural Network Processing:** All three amp models (Matchless SC30, Klon, Mesa Boogie IIB) with identical characteristics
* **Filter Implementation:** Faithful reproduction of original Mars.cpp tone filtering with cubic parameter curves and exact frequency ranges (100Hz-20kHz low-pass, 40Hz-440Hz high-pass)
* **Cabinet Simulation:** Three impulse response models (Bright, Neutral, Dark)
* **Delay Processing:** Normal, dotted-eighth, and triplet delay modes with original timing calculations
* **Signal Processing:** Energy-constant crossfade mixing and proper volume compensation
* **Hardware Integration:** Full utilization of Hothouse's 6 knobs, 3 toggle switches, and 2 footswitches

### Features Not Yet Implemented:
Due to platform differences between Funbox and Hothouse, the following features are not currently available:

* **Preset Save/Load System:** Footswitch 2 functionality for saving and loading presets (requires persistent storage implementation)
* **Expression Pedal Control:** Hothouse hardware doesn't include expression pedal input
* **Dip Switch Settings:** Hothouse lacks physical dip switches, so the following settings are hard-coded:
  - Neural model processing: **Enabled** (dipValues[0] = true)
  - Impulse response processing: **Enabled** (dipValues[1] = true)  
  - Mono/Stereo mode: **Mono** (dipValues[2] = false)
  - Additional setting: **Disabled** (dipValues[3] = false)

### Future Development:
These features could be implemented in future versions:
* Persistent storage for preset save/load functionality
* Alternative control methods for settings currently requiring dip switches
* Expression control via MIDI or alternative input methods

## Platform Acknowledgments

### Funbox Platform:
The original Funbox platform by GuitarML provides a comprehensive framework for guitar effect development on the Daisy Seed, with standardized hardware interfaces and proven DSP algorithms.

### Hothouse Platform:
The Cleveland Music Co. Hothouse platform provides professional-grade hardware abstraction for the Daisy Seed, enabling rapid development of guitar effects with robust hardware control.

### Underlying Technology:
* **Daisy Seed:** Electro-Smith audio processing platform
* **libDaisy:** Hardware abstraction library
* **DaisySP:** Digital signal processing library
* **RTNeural:** Real-time neural network inference library

## License Information

This Mars Hothouse port is distributed under the same MIT License with Attribution terms as the original Mars project. 

See the included `funbox_mars_original_license.txt` file for complete license details.

## Credits and Thanks

* **GuitarML Team:** For creating the original Mars neural amplifier simulator and the comprehensive Funbox platform
* **Keith Bloemer:** Original developer of the Funbox platform and Mars effect
* **Cleveland Music Co.:** For developing the Hothouse hardware platform and providing development support
* **Electro-Smith:** For the Daisy Seed ecosystem that enables both platforms
* **Open Source Community:** For the libraries and tools that make these projects possible

## How to Contribute

While this is a binary distribution, the conversion techniques and DSP implementations may be valuable for other Funbox-to-Hothouse ports. Consider contributing to:

* **Original Mars Project:** https://github.com/GuitarML/FunBox
* **Hothouse Platform:** [Cleveland Music Co. repositories]
* **Daisy Community:** https://forum.electro-smith.com/

## Contact

For questions about this Hothouse port, please refer to the Mars Hothouse installation guide and troubleshooting documentation included in this package.

For questions about the original Mars implementation, please visit the GuitarML Funbox repository.