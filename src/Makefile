# Logic and custom rules to make copying the *.bin files
# easy to script on various OSes

# HACK: there are more elegant ways to do this, but it's simple
# and it works for an unsophiticated binary publishing hook

# Define the file to copy and the destination directory
# We know that BUILD_DIR and TARGET are already set
SRC_FILE := $(BUILD_DIR)/$(TARGET).bin
DEST_FILE := $(PUBLISH_DIR)/$(TARGET).bin

# Use a variable to determine the OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    OS := Linux
else ifeq ($(UNAME_S),Darwin)
    OS := macOS
else ifeq ($(OS),Windows_NT)
    OS := Windows
else
    OS := Unknown
endif

# Normalize paths based on the OS
ifeq ($(OS),Windows)
    NORM_SRC_FILE := $(subst /,\,$(SRC_FILE))
    NORM_DEST_FILE := $(subst /,\,$(DEST_FILE))
else
    NORM_SRC_FILE := $(subst \,/,$(SRC_FILE))
    NORM_DEST_FILE := $(subst \,/,$(DEST_FILE))
endif

# Define a copy command based on the OS
ifeq ($(OS),macOS)
    COPY_CMD := cp -p
else ifeq ($(OS),Linux)
    COPY_CMD := cp
else ifeq ($(OS),Windows)
    COPY_CMD := copy
endif

# Target rule to copy the file
# External scripts trigger like this:
#   make publish PUBLISH_DIR=/foo/bar
publish:
	$(COPY_CMD) $(NORM_SRC_FILE) $(NORM_DEST_FILE)

.PHONY: publish
