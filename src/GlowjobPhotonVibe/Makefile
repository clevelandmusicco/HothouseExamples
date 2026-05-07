# Project Name
TARGET = glowjob_photon_vibe

# Sources
CPP_SOURCES = glowjob_photon_vibe.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP
HOTHOUSE_DIR = ..

# Hothouse hardware abstraction is shared across examples in src/.
C_INCLUDES += -I$(HOTHOUSE_DIR)
CPP_SOURCES += $(HOTHOUSE_DIR)/hothouse.cpp

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
include ../Makefile
