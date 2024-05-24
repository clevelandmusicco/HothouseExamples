# Examples for the Hothouse DIY DSP Platform

The Cleveland Music Co. Hothouse DIY DSP Platform is a compact pedal interface for the [Electrosmith Daisy Seed DSP](https://electro-smith.com/products/daisy-seed).

This project is a collection of digital signal processing code examples that you can use to get started with the Hothouse. In the `src` directory is a collection of ready-to-go effects you can flash to your Hothouse or modify as you wish:

* **ModDelay** - Modulated delay with a 5-minute looper and vibrato or chorus modes
* **ShimmerVerb** - Shimmer reverb with modulated reverb tails
* **MegaTremolo** - Tremolo with fully-tweakable harmonic tremolo mode
* **TriChorus** - Chorus effect with up to three choris voices
* And more to come ...

Also included is a `create_new_proj.py` helper script that creates a compilable, VS Code-ready "scaffolding" project for writing your own effects for the Hothouse.

If you're not familar with the Daisy Seed or its development environment, check out the [Electrosmith Daisy Ecosystem Wiki](https://github.com/electro-smith/DaisyWiki/wiki).

## Prerequisites

* **[A Daisy development environment](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment)** - Nothing that comes after this will work until you have the Daisy toolchain installed, configured, and functioning properly.
* **A Cleveland Music Co. Hothouse (with a Daisy Seed installed)** - Whether you acquired it as a kit or fully-assembled, either will work fine.

## Building the code

Clone the repo:

```bash
git clone https://github.com/clevelandmusicco/HothouseExamples
```

Init and build the [libDaisy](https://github.com/electro-smith/libDaisy.git) and [DaisySP](https://github.com/electro-smith/DaisySP.git) libraries; these are included as submodules:

```bash
cd HothouseExamples
git submodule update --init --recursive

make -C libDaisy
make -C DaisySP
```

To build all of the effects in the `src` dir:

```bash
python build_examples.py
```

To (re)build a specific effect (replace 'HelloWorld' with the desired effect):

```bash
cd src/HelloWorld
make clean; make
```

## Flashing the Hothouse

To flash an effect to your Hothouse, you will be loading a compiled binary on to the Daisy Seed. `cd` into the desired effect directory:

```bash
cd src/HelloWorld
```

Assuming you've already compiled the code, enter bootloader mode on your Daisy Seed (see pic [here](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4a-flashing-the-daisy-via-usb)) and flash with with the following command:

```bash
# Using USB
make program-dfu
```

If you're using a [JTAG/SWD debugger](https://electro-smith.com/products/st-link-v3-mini-debugger) (*and we highly recommend you do so if you're doing development work!*) there's no need to enter bootloader mode on the Daisy Seed. Simply run this command:

```bash
# Using JTAG/SWD adaptor (like STLink)
make program
```

## Daisy Web Programmer

Alternatively, you can flash the Daisy Seed using the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/).

## Creating your own effect

Use the `create_new_proj.py` helper script to create a bare effect project in the `src` dir:

```bash
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

`--your_name` and `--your_email` are optional. If they are omitted, "Your Name" and "your@email" will be used in the new project code.

```bash
python create_new_proj.py --proj_name MyAwesomeEffect --your_name "John Developer" --your_email john@developer.io
```

This results in a new directory under `src`:

```bash
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

The template project the `create_new_proj.py` script copies from is in `resources/_template'`.

Straight away, the code can be compiled and flashed as usual, but it simply writes silence to the output.

```bash
cd src/MyAwesomeEffect
make clean
make

# USB
make program-dfu

# JTAG/SWD
make program
```

## VS Code

Any of the effect projects in the `src` directory can be opened in VS Code. Simply use the `Open Folder...` option and select the effect directory (**NOT** the `HothouseExamples` or `src` directory):

![The MyAwesomeEffect project in VS Code](resources/images/new_proj_vscode.png)

This ensures that the tasks in `tasks.json` and the debug executable in `launch.json` work properly.
