# Inky Display Library Makefile
# Supports hardware-only, emulator, and test builds

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99 -D_GNU_SOURCE -I./include
LDFLAGS = -lm

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = test
BUILD_DIR = build
LIB_DIR = lib

# Source files
COMMON_SRC = $(SRC_DIR)/inky_common.c
HARDWARE_SRC = $(SRC_DIR)/inky_hardware.c
EMULATOR_SRC = $(SRC_DIR)/inky_emulator.c

# Object files
COMMON_OBJ = $(BUILD_DIR)/inky_common.o
HARDWARE_OBJ = $(BUILD_DIR)/inky_hardware.o
EMULATOR_OBJ = $(BUILD_DIR)/inky_emulator.o

# Library targets
LIBINKY_HW = $(LIB_DIR)/libinky_hw.a
LIBINKY_EMU = $(LIB_DIR)/libinky_emu.a

# Test executables
TEST_MODELS = $(BUILD_DIR)/test_models

# Default target
.PHONY: all
all: emulator

# Create build directories
$(BUILD_DIR) $(LIB_DIR):
	mkdir -p $@

# Compile common source (used by both hardware and emulator)
$(COMMON_OBJ): $(COMMON_SRC) $(INC_DIR)/inky.h $(INC_DIR)/inky_internal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile hardware source
$(HARDWARE_OBJ): $(HARDWARE_SRC) $(INC_DIR)/inky.h $(INC_DIR)/inky_internal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile emulator source
$(EMULATOR_OBJ): $(EMULATOR_SRC) $(INC_DIR)/inky.h $(INC_DIR)/inky_internal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build hardware-only library (for Raspberry Pi - Linux only)
.PHONY: hardware
hardware:
	@if [ "$$(uname)" != "Linux" ]; then \
		echo "Error: Hardware library can only be built on Linux (Raspberry Pi)"; \
		echo "The hardware implementation requires Linux-specific SPI, GPIO, and I2C interfaces."; \
		echo "Use 'make emulator' to build the emulator library for development on macOS/other platforms."; \
		exit 1; \
	fi
	@$(MAKE) $(LIBINKY_HW)

$(LIBINKY_HW): $(COMMON_OBJ) $(HARDWARE_OBJ) | $(LIB_DIR)
	ar rcs $@ $^
	@echo "Hardware library built: $@"
	@echo "Link with: -L./lib -linky_hw"

# Build emulator library (for development/testing)
.PHONY: emulator
emulator: $(LIBINKY_EMU)

$(LIBINKY_EMU): $(COMMON_OBJ) $(EMULATOR_OBJ) | $(LIB_DIR)
	ar rcs $@ $^
	@echo "Emulator library built: $@"
	@echo "Link with: -L./lib -linky_emu"

# Build test programs
.PHONY: test
test: $(TEST_MODELS)

$(TEST_MODELS): $(TEST_DIR)/test_models.c $(LIBINKY_EMU) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L./$(LIB_DIR) -linky_emu $(LDFLAGS) -o $@
	@echo "Test executable built: $@"

# Run tests
.PHONY: run-test
run-test: $(TEST_MODELS)
	@echo "Running display model tests..."
	@cd $(BUILD_DIR) && ./test_models
	@echo ""
	@echo "Test images generated in $(BUILD_DIR)/"

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)
	@echo "Cleaned build artifacts"

# Clean everything including test outputs
.PHONY: distclean
distclean: clean
	rm -f $(TEST_DIR)/*.ppm
	@echo "Cleaned all generated files"

# Help target
.PHONY: help
help:
	@echo "Inky Display Library - Build Targets:"
	@echo ""
	@echo "  make hardware     - Build hardware-only library (libinky_hw.a)"
	@echo "                      Use this for Raspberry Pi deployment"
	@echo ""
	@echo "  make emulator     - Build emulator library (libinky_emu.a) [default]"
	@echo "                      Use this for development and testing"
	@echo ""
	@echo "  make test         - Build test programs using emulator"
	@echo ""
	@echo "  make run-test     - Build and run test programs"
	@echo ""
	@echo "  make all          - Build emulator (same as 'make emulator')"
	@echo ""
	@echo "  make clean        - Remove build artifacts"
	@echo ""
	@echo "  make distclean    - Remove all generated files including test images"
	@echo ""
	@echo "Library API: include/inky.h"
	@echo ""
	@echo "Examples:"
	@echo "  Compile for hardware: make clean && make hardware"
	@echo "  Compile for testing:  make clean && make test run-test"

.PHONY: all emulator hardware test run-test clean distclean help
