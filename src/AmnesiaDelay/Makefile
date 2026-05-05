# Project Name
TARGET = amnesia_delay

# Sources
CPP_SOURCES = amnesia_delay.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP
HOTHOUSE_DIR = ..

# Hothouse hardware abstraction is shared across examples in src/.
C_INCLUDES += -I$(HOTHOUSE_DIR)
CPP_SOURCES += $(HOTHOUSE_DIR)/hothouse.cpp

# Use DaisySP-LGPL components if you ever need them (e.g. ReverbSc).
# USE_DAISYSP_LGPL = 1

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
