# Venus Spectral Reverb - Hothouse Port

## ORIGINAL PROJECT
- **Name:** Venus Spectral Reverb
- **Platform:** Funbox (Daisy Seed)
- **Repository:** https://github.com/GuitarML/FunBox/tree/main/software/Venus
- **License:** MIT

## SPECTRAL REVERB ALGORITHM
- **Concept:** Based on spectral reverb techniques by geraintluff
- **Forum:** https://forum.cockos.com/showthread.php?t=225955
- **Implementation:** FFT-based frequency domain processing

## HOTHOUSE PORT
- **Porter:** Chris Brandt
- **Contact:** chris@futr.tv
- **Date:** September 17, 2025
- **Version:** 1.0
- **Changes:** 
  - Adapted hardware interface for Hothouse platform
  - Converted from Funbox control scheme
  - Optimized for Hothouse toggle switch mapping
  - Removed expression pedal and MIDI functionality

## THIRD-PARTY LIBRARIES
- **DaisySP:** Electrosmith (MIT License)
- **libDaisy:** Electrosmith (MIT License)
- **shy_fft:** Custom FFT implementation
- **soundmath:** Fourier transform utilities

## ACKNOWLEDGMENTS
Special thanks to:
- The GuitarML team for the original Venus
- geraintluff for spectral reverb concepts
- Electrosmith for the Daisy platform
- Cleveland Music Co. for the Hothouse hardware

## COPYRIGHT NOTICE
Copyright (c) 2023-2024 GuitarML Team  
Copyright (c) 2025 Chris Brandt (Hothouse port)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.