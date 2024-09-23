# Project Name
TARGET = stereo_reverb_with_tails

# Uncomment to use LGPL (like ReverbSc, etc.)
USE_DAISYSP_LGPL=1

# Sources and Hothouse header files
CPP_SOURCES = stereo_reverb_with_tails.cpp ../hothouse.cpp
C_INCLUDES = -I..

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Global helpers
include ../Makefile
