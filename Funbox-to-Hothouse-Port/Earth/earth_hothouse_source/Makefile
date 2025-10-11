# Earth Reverbscape - Hothouse Port
# Makefile for building the Earth multi-effect pedal
# Original: Keith Bloemer (GuitarML)
# Hothouse Port: Chris Brandt, September 5, 2025

# Project Name
TARGET = earth_hothouse

# C++ 20 required for std::span
CPP_STANDARD = -std=c++20

# Use LGPL version of DaisySP
USE_DAISYSP_LGPL = 1

# Memory configuration - SRAM mode (default, no APP_TYPE specified)
# Uncomment the line below to use QSPI with bootloader:
# APP_TYPE = BOOT_QSPI

# Compiler options
OPT = -Ofast -fno-strict-aliasing

# Sources - MUST include hothouse.cpp
CPP_SOURCES = earth_hothouse.cpp hothouse.cpp
CPP_SOURCES += Dattorro/dsp/filters/OnePoleFilters.cpp
CPP_SOURCES += Dattorro/dsp/delays/InterpDelay.cpp
CPP_SOURCES += Dattorro/Dattorro.cpp

# Library Locations
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP

# Core location, and generic Makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Include directories - all local now
C_INCLUDES += -I.
C_INCLUDES += -Iq/q_lib/include
C_INCLUDES += -Igcem/include
C_INCLUDES += -Iinfra/include
