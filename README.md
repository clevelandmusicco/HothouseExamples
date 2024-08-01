# Examples for the Hothouse Digital Signal Processing Pedal Kit

**:wrench: [Hothouse Build Guide](https://github.com/clevelandmusicco/HothouseExamples/wiki/Cleveland-Music-Co.-Hothouse-DIY-Digital-Signal-Processing-Pedal-Kit-Build-Guide) | :question: [FAQ](https://github.com/clevelandmusicco/HothouseExamples/wiki/Frequently-Asked-Questions) | :books: [Wiki](https://github.com/clevelandmusicco/HothouseExamples/wiki)**

The [Cleveland Music Co. Hothouse](https://clevelandmusicco.com/hothouse-diy-digital-signal-processing-platform-kit/) is a compact pedal kit for the [Electrosmith Daisy Seed DSP](https://electro-smith.com/products/daisy-seed). You can use the Hothouse to easily get your Daisy Seed DSP projects off the breadboard and onto your pedalboard, and / or you can simply compile and flash any of the code in this repo to your Hothouse.

<img src="/resources/hothouse-front-553px.png" alt="Cleveland Music Co. Hothouse Pedal" style="height:400px; width:400px;"/><img src="/resources/hothouse-kit-553px.png" alt="Cleveland Music Co. Hothouse Kit" style="height:400px; width:400px;"/>

This project is a collection of digital signal processing code examples that you can use to get started with the Hothouse. In the `src` directory are ready-to-compile effects you can flash to your Hothouse or modify as you wish:

* **ModDelay** - Coming soon ... Modulated delay with vibrato or chorus mode, and a 5-minute looper
* **ShimmerVerb** - Shimmer reverb with modulated reverb tails
* **HarmonicTremVerb** - Tremolo with tweakable harmonic mode and a spring reverb effect
* **TriChorus** - Coming soon ... Chorus effect with up to three chorus voices
* And more to come ...

## Community contributions

* **TremVerb** - Tremolo / reverb effect contributed by by [tele_player](https://forum.electro-smith.com/u/tele_player/summary) on the [Electrosmith Forums](https://forum.electro-smith.com/t/hothouse-dsp-pedal-kit/5631/14).

> [!NOTE]
> This repo is in its early days. Over time, it will grow with contributions from Cleveland Music Co., as well as&mdash;if all goes well&mdash;many more contributions from the community!

Also included is a `create_new_proj.py` helper script that creates a compilable, VS Code-ready "scaffolding" project for writing your own effects for the Hothouse.

If you're not familar with the Daisy Seed or its development environment, check out the [Electrosmith Daisy Ecosystem Wiki](https://github.com/electro-smith/DaisyWiki/wiki).

## A note for the ~~neckbeards~~ perfectionists

The code in this project is meant to strike a balance between educational and functional. There are more elegant, concise, and sometimes "better" ways to accomplish what much of this code does. However, in the interest of accessibility and understandability, it is presented in a simple, straightforward form. If you want to improve the code in ways that make it more educational and functional, please do so! Pull requests are always welcome.

## Getting started

### Prerequisites

* **[A Daisy Seed with 65MB of memory](https://electro-smith.com/products/daisy-seed?variant=45234245108004)** - While the 65MB is not critical, it is highly recommended. Several of the examples in this repo will not compile on the 1MB version of the Daisy Seed. Just spend the extra few dollars for the additional capacity.
* **[A Daisy development environment](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment)** - Nothing that comes after this will work until you have the Daisy toolchain installed, configured, and functioning properly. We use Linux for development and testing, but the commands on this README page should be cross-platform if your toolchain is configured properly.
* **[A Cleveland Music Co. Hothouse](https://clevelandmusicco.com/hothouse-diy-digital-signal-processing-platform-kit/) (with a Daisy Seed installed)** - Whether you acquired it as a kit or fully-assembled, either will work fine.
* **Python 3.x** - The commands on this page were tested with `Python 3.10.14` aliased to the local `python` command. The python scripts in this repo have not been tested with any other version.

### Building the code

Clone the repo:

```console
git clone https://github.com/clevelandmusicco/HothouseExamples
```

Init and build the [libDaisy](https://github.com/electro-smith/libDaisy.git) and [DaisySP](https://github.com/electro-smith/DaisySP.git) libraries; these are included as submodules:

```console
cd HothouseExamples
git submodule update --init --recursive

make -C libDaisy
make -C DaisySP
```

To build all of the effects in the `src` dir:

```console
python build_examples.py
```

To (re)build a specific effect (replace 'HelloWorld' with the desired effect):

```console
cd src/HelloWorld
make clean; make
```

### Flashing the Hothouse

To flash an effect to your Hothouse, you will load a compiled binary on to the Daisy Seed. `cd` into the desired effect directory:

```console
cd src/HelloWorld
```

Assuming you've already compiled the code, enter bootloader mode on your Daisy Seed (see pic [here](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4a-flashing-the-daisy-via-usb)) and flash with with the following command:

```console
# Using USB
make program-dfu
```

Install the Daisy Seed into your Hothouse, power it up and off you go!

If you're using a [JTAG/SWD debugger](https://electro-smith.com/products/st-link-v3-mini-debugger) (***AND WE HIGHLY RECOMMEND YOU DO** if you're doing development work!*) there's no need to enter bootloader mode on the Daisy Seed. Simply run this command with your debugger attached:

```console
# Using JTAG/SWD adaptor (like STLink)
make program
```

> [!TIP]
> An added convenience when using the JTAG debugger / programmer is that you don't need to remove and reinstall the Daisy Seed to flash it; you can easily leave the Daisy Seed installed in the Hothouse while debugging or programming.

### Daisy Web Programmer

Alternatively, you can flash the Daisy Seed using the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/).

### Creating your own effect

Use the `create_new_proj.py` helper script to create a bare effect project in the `src` dir:

```console
python create_new_proj.py -h
usage: create_new_proj.py [-h] --proj_name PROJ_NAME [--your_name YOUR_NAME] [--your_email YOUR_EMAIL]

options:
  -h, --help            show this help message and exit
  --proj_name PROJ_NAME
                        Name of the new project in camelCase or PascalCase.
  --your_name YOUR_NAME
                        Your name for use in the license and README.
  --your_email YOUR_EMAIL
                        Your email address for use in the license and README.

```

> [!NOTE]
> `--your_name` and `--your_email` are optional. If they are omitted, "Your Name" and "your@email" will be used in the new project code.

```console
python create_new_proj.py --proj_name MyAwesomeEffect \
                          --your_name "John Developer" \
                          --your_email john.developer@email.domain
```

This results in a new directory under `src`:

```console
src/MyAwesomeEffect
├── Makefile
├── my_awesome_effect.cpp
├── README.md
└── .vscode
    ├── c_cpp_properties.json
    ├── .cortex-debug.peripherals.state.json
    ├── .cortex-debug.registers.state.json
    ├── launch.json
    ├── STM32H750x.svd
    └── tasks.json
```

Straight away, the code can be compiled and flashed as usual, but until you add your own code, it will just write silence to the output.

```cpp
void AudioCallback(AudioHandle::InputBuffer in, 
                  AudioHandle::OutputBuffer out,
                  size_t size) {

  // Stuff omitted for brevity...

  for (size_t i = 0; i < size; ++i) {
    if (bypass) {
      out[0][i] = in[0][i];
    } else {
      out[0][i] = 0.0f;  // TODO: replace silence with something awesome
    }
  }
}
```

> [!NOTE]
> Notice the `TODO` comment above! Once you've replaced the code with your own, build and flash as per normal.

```console
cd src/MyAwesomeEffect
make clean
make

# USB
make program-dfu

# JTAG/SWD
make program
```

> [!TIP]
> The `create_new_proj.py` script copies a template project while replacing some string tokens along the way. The template project is in `resources/_template` and can be modified / extended to your liking.

### VS Code

Any of the effect projects in the `src` directory can be opened in VS Code. Use the `Open Folder...` option and select the effect directory (**NOT** the `HothouseExamples` or `src` directory):

![The MyAwesomeEffect project in VS Code](resources/images/new_proj_vscode.png)

This ensures that the tasks in `tasks.json` and the debug executable in `launch.json` work properly.
