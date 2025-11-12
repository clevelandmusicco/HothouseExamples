# Project Name
TARGET = venus_hothouse

USE_DAISYSP_LGPL = 1

# Sources
CPP_SOURCES = venus_hothouse.cpp hothouse.cpp

# Optimization level
OPT = -O2

# Library Locations (adjust these paths to match your setup)
LIBDAISY_DIR = ../../../libDaisy
DAISYSP_DIR = ../../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Add current directory to include path for local headers
C_INCLUDES += -I.