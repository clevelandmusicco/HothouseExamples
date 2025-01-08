# Project Name
TARGET = flick

# Uncomment to use LGPL (like ReverbSc, etc.)
#USE_DAISYSP_LGPL=1

# Sources and Hothouse header files
CPP_SOURCES = flick.cpp flick_oscillator.cpp ../hothouse.cpp

# Add PlateauNEVersio sources
CPP_SOURCES += PlateauNEVersio/utilities/Utilities.cpp
CPP_SOURCES += PlateauNEVersio/dsp/filters/OnePoleFilters.cpp
CPP_SOURCES += PlateauNEVersio/dsp/delays/InterpDelay.cpp
CPP_SOURCES += PlateauNEVersio/Dattorro.cpp

C_INCLUDES = -I.. -I./PlateauNEVersio

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Global helpers
include ../Makefile
