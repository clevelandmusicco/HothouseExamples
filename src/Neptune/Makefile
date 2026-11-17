# Project Name
TARGET = neptune

# 1 = true stereo in; 0 = mono in (left jack) feeding the stereo reverb
NEPTUNE_TRUE_STEREO ?= 0
C_DEFS = -DNEPTUNE_TRUE_STEREO=$(NEPTUNE_TRUE_STEREO)

# Tone (octave delay lowpass) lives in DaisySP-LGPL
USE_DAISYSP_LGPL = 1

# Small app at -Os, CloudSeed DSP at -O3
OPT = -Os

CLOUDSEED_SOURCES = \
	cloudseed/FastSin.cpp \
	cloudseed/AudioLib/Biquad2.cpp \
	cloudseed/AudioLib/ShaRandom.cpp \
	cloudseed/AudioLib/ValueTables.cpp \
	cloudseed/Utils/Sha256.cpp

# Sources and Hothouse header files
CPP_SOURCES = neptune.cpp ../hothouse.cpp $(CLOUDSEED_SOURCES)
# -isystem: keep vendored CloudSeed warnings out of our build output
C_INCLUDES = -I.. -isystem cloudseed

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CLOUDSEED_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(CLOUDSEED_SOURCES:.cpp=.o)))
$(CLOUDSEED_OBJECTS): OPT = -O3
$(CLOUDSEED_OBJECTS): CPPFLAGS += -w

# Global helpers
include ../Makefile
