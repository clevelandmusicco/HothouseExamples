# Examples for the Hothouse DIY DSP Platform

The Cleveland Music Co. Hothouse DIY DSP Platform is a compact pedal interface for the [Electrosmith Daisy Seed DSP](https://electro-smith.com/products/daisy-seed).

This project is a collection of digital signal processing code examples that you can use to get started with Hothouse. It provides a minimal template for creating your own effects, as well as a growing collection of popular effects you can flash on to your Hothouse or modify as you wish.

If you're not familar with the Daisy Seed or its development environment, checkout the [Electrosmith Daisy Ecosystem Wiki](https://github.com/electro-smith/DaisyWiki/wiki).

## Prerequisites

* [A Daisy development environment](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment). Nothing that comes after this will work until you have the Daisy toolchain installed, configured, and functioning properly.
* A Cleveland Music Co. Hothouse. Whether you acquired one as a kit or fully-assembled, either will work fine.

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

Build all of the examples:

```bash
python build_examples.py
```

To (re)build a specific example (replace 'HelloWorld' with the desired example):

```bash
cd src/HelloWorld
make clean; make
```

## Flashing the Hothouse

To flash an example to your Hothouse, you will be loading a compiled binary on to the Daisy Seed. `cd` into the desired example directory:

```bash
cd src/HelloWorld
```

Then, enter bootloader mode on your Daisy Seed (see pic [here](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4a-flashing-the-daisy-via-usb)) and flash with with the following command:

```bash
# Using USB
make program-dfu
```

If you're using a [JTAG/SWD debugger](https://electro-smith.com/products/st-link-v3-mini-debugger) (and we highly recommend you do if you're doing development work!) there's no need to enter bootloader mode on the Daisy Seed. Simply run this command:

```bash
# Using JTAG/SWD adaptor (like STLink)
make program
```

## Daisy Web Programmer

Alternatively, you can flash the Daisy Seed using the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/).
