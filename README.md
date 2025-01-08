# Examples for Hothouse Digital Signal Processing Pedal Kit

**:wrench: [Hothouse Build Guide](https://github.com/clevelandmusicco/HothouseExamples/wiki/Cleveland-Music-Co.-Hothouse-DIY-Digital-Signal-Processing-Pedal-Kit-Build-Guide-(Stereo-Version)) | :rocket: [10-Minute Quick Start](https://github.com/clevelandmusicco/HothouseExamples/wiki/10%E2%80%90Minute-Quick-Start) | :page_facing_up: [Code Examples](https://github.com/clevelandmusicco/HothouseExamples/wiki/Hothouse-Examples) | :books: [Wiki](https://github.com/clevelandmusicco/HothouseExamples/wiki) | :question: [FAQ](https://github.com/clevelandmusicco/HothouseExamples/wiki/Frequently-Asked-Questions)**

The [Cleveland Music Co. Hothouse](https://clevelandmusicco.com/hothouse-diy-digital-signal-processing-platform-kit/) is a compact pedal kit for the [Electrosmith Daisy Seed](https://electro-smith.com/products/daisy-seed). You can use the Hothouse to easily get your Daisy Seed DSP projects off the breadboard and onto your pedalboard, either by compiling and flashing any of the code in this repository, or by using the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/) with the binaries from [our latest release](https://github.com/clevelandmusicco/HothouseExamples/releases). And of course, you can also [write your own DSP code](https://github.com/clevelandmusicco/HothouseExamples/wiki/Creating-your-own-effects) for the Hothouse.

<img src="./resources/hothouse-front-553px.png" alt="Cleveland Music Co. Hothouse Pedal" style="height:400px; width:400px;"/><img src="./resources/hothouse-kit-553px.png" alt="Cleveland Music Co. Hothouse Kit" style="height:400px; width:400px;"/>

This project is a collection of digital signal processing code examples that you can use to get started with the Hothouse. In the `src` directory are ready-to-compile effects you can flash to your Hothouse or modify as you wish. Also included is a `create_new_proj.py` helper script that creates a compilable, VS Code-ready "scaffolding" project for writing your own effects for the Hothouse.

If you're not familar with the Daisy Seed or its development environment, check out the [Electrosmith Daisy Ecosystem Wiki](https://github.com/electro-smith/DaisyWiki/wiki) to get yourself bootstrapped! Also be sure to bookmark the [Daisy Forum](https://forum.electro-smith.com/) and join the [Discord server](https://discord.gg/SuCtUsbD) to hang out with like-minded people committed to collective learning.

## Getting started

* [**Hothouse Build Guide**](https://github.com/clevelandmusicco/HothouseExamples/wiki/Cleveland-Music-Co.-Hothouse-DIY-Digital-Signal-Processing-Pedal-Kit-Build-Guide-(Stereo-Version)) - The official build documentation for the current Hothouse hardware. Whether you have a kit or a pre-assembled pedal, this page walks you through getting the Hothouse ready for programming.
* [**10-Minute Quick Start**](https://github.com/clevelandmusicco/HothouseExamples/wiki/10%E2%80%90Minute-Quick-Start) - Once you've got an assembled Hothouse pedal in-hand, this is your next stop. A guide that has you flashing the pedal and making noise in 10-minutes or less.
* [**List of Hothouse Examples**](https://github.com/clevelandmusicco/HothouseExamples/wiki/Hothouse-Examples) - A definitive (and growing!) list of the available examples, loosely-organized and with brief descriptions.
* [**Creating Your Own Effects**](https://github.com/clevelandmusicco/HothouseExamples/wiki/Creating-your-own-effects) - Ready to move on from the examples and roll your own effects? This page introduces the `create_new_proj.py` helper script and shares some basic DSP tips.
* [**About "Stereo" and "Mono" Audio Modes**](https://github.com/clevelandmusicco/HothouseExamples/wiki/About-%22Stereo%22-and-%22Mono%22-Audio-Modes) - _Mono-to-mono? Mono-to-stereo?_ What's all this then? Read about audio modes supported by the Hothouse and see code that implements each mode.
* [**Using the `build_examples.py` helper script**](https://github.com/clevelandmusicco/HothouseExamples/wiki/Using-the-build_examples.py-helper-script) - A brief explanation of how to use the helper script and why you might want to.
* [**Frequently Asked Questions**](https://github.com/clevelandmusicco/HothouseExamples/wiki/Frequently-Asked-Questions) - A growing list of questions and answers. Check here first if you're having trouble figuring something out.

## Third-Party Support

The Hothouse platform is supported by several popular DSP projects.

* [Faust](https://github.com/grame-cncm/faust) - Programming Language for Audio Applications and Plugins. The [faust2hothouse tool](https://github.com/grame-cncm/faust/tree/master-dev/architecture/hothouse) compiles a Faust DSP program for the Hothouse. The script produces a folder containing the C++ source code and a Makefile to compile it. It even implements MIDI over USB!
* [plugdata](https://plugdata.org/) - A visual programming environment for audio experimentation, prototyping and education. [plugdata is a free/open-source visual programming environment](https://github.com/plugdata-team/plugdata) based on pure-data. It allows you to create and manipulate audio systems using visual elements, rather than writing code. Exporting your Pd patch directly to the Hothouse is possible with plugdata, which uses [HVCC](https://github.com/Wasted-Audio/hvcc) and the [plugdata-heavy-toolchain](https://github.com/plugdata-team/plugdata-heavy-toolchain) to make the magic happen. Just select the Hothouse from the target board dropdown and you're off to the races!

| 💥 Also Good To Know 💥 |
|-|
| For the sake of simplicity, the examples in this repository are written with a focus on the use of [DaisySP classes](https://electro-smith.github.io/DaisySP/annotated.html) whenever available. There are lots of third-party libre and [open-source DSP libraries](https://search.brave.com/search?q=open+source+%28inpage%3Adsp+OR+inpage%3A%22digital+signal+processing%22%29+libraries&source=web) out there that provide more advanced processing, but using them can be complicated and daunting for the uninitiated. These examples are about demonstrating the use of the Hothouse itself with as little distraction as is reasonable. That's not to say there won't be more advanced projects added at a later date, though ... |
