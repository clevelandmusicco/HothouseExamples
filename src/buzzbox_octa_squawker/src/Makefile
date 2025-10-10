# Project Name
TARGET = buzzbox_hothouse

# C++ 20 required for std::span (octave processing)
CPP_STANDARD = -std=c++20

# Use DaisySP LGPL modules
USE_DAISYSP_LGPL = 1

# Memory configuration
# Comment out APP_TYPE to use default SRAM mode (no bootloader needed)
# For production with bootloader, uncomment and set to BOOT_QSPI
# APP_TYPE = BOOT_QSPI

# Compiler optimization - use -Ofast for maximum performance
OPT = -Ofast -fno-strict-aliasing

# Sources - Includes main application and Hothouse hardware library
CPP_SOURCES = buzzbox_hothouse.cpp hothouse.cpp

# Library Locations (adjust these paths based on your workspace setup)
# For HothouseExamples structure: src/buzzbox_octa_squawker_source/src/
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP

# Core location, and generic Makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Include directories
# Current directory for local headers
C_INCLUDES += -I.

# Q DSP library for octave generation
C_INCLUDES += -I../lib/q/q_lib/include

# GCEM library for compile-time math
C_INCLUDES += -I../lib/gcem/include

# Infra library
C_INCLUDES += -I../lib/infra/include

# Parent directory to resolve Util/Multirate.h and Util/OctaveGenerator.h
# The code uses #include "Util/Multirate.h", so we need -I.. not -I../Util
C_INCLUDES += -I..
