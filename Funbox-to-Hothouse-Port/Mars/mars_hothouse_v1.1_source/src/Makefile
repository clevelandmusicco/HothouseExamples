# Project Name
TARGET = mars_hothouse

USE_DAISYSP_LGPL = 1

# Memory configuration - COMMENT OUT until bootloader is installed
# APP_TYPE = BOOT_QSPI
# Uncomment above line AFTER running 'make program-boot'

# Compiler options - use -Ofast for maximum performance (Mars developer recommendation)
OPT = -Ofast

# Sources - MUST include hothouse.cpp and all IR-related sources
CPP_SOURCES = mars_hothouse.cpp hothouse.cpp ImpulseResponse/ImpulseResponse.cpp ImpulseResponse/dsp.cpp

# Library Locations
LIBDAISY_DIR = ../../../../libDaisy
DAISYSP_DIR = ../../../../DaisySP
RTNEURAL_DIR = RTNeural
# Alternative paths if RTNeural is elsewhere:
# RTNEURAL_DIR = ../RTNeural
# RTNEURAL_DIR = /path/to/RTNeural

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Include directories
C_INCLUDES += -I. -I$(RTNEURAL_DIR) -I$(RTNEURAL_DIR)/modules/Eigen
CPPFLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=8 -DRTNEURAL_NO_DEBUG=1
