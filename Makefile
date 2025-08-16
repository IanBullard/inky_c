CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99 -D_GNU_SOURCE
LDFLAGS = 

# Output directories
BUILD_DIR = build
BIN_DIR = bin

# Create directories
$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR))

# Emulator build (works on any platform)
EMULATOR_TARGET = $(BIN_DIR)/test_clear_emulator
EMULATOR_OBJS = $(BUILD_DIR)/test_clear.o $(BUILD_DIR)/inky_emulator.o

# Hardware build (Linux only)
HARDWARE_TARGET = $(BIN_DIR)/test_clear_hardware
HARDWARE_OBJS = $(BUILD_DIR)/test_clear_hw.o $(BUILD_DIR)/inky_hardware.o

# Default target - build emulator version
all: emulator

# Emulator version
emulator: $(EMULATOR_TARGET)

$(EMULATOR_TARGET): $(EMULATOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built emulator version: $@"

$(BUILD_DIR)/inky_emulator.o: inky_emulator.c inky.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Hardware version (Raspberry Pi only)
hardware: 
	@if [ "$$(uname)" != "Linux" ]; then \
		echo "Error: Hardware version can only be built on Raspberry Pi (Linux ARM)"; \
		echo "The hardware implementation requires Raspberry Pi SPI and GPIO interfaces."; \
		echo "Use 'make emulator' to build the emulator version on all other platforms."; \
		exit 1; \
	fi
	@echo "Building hardware version for Raspberry Pi..."
	@$(MAKE) $(HARDWARE_TARGET)

$(HARDWARE_TARGET): $(HARDWARE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built hardware version: $@"

$(BUILD_DIR)/inky_hardware.o: inky_hardware_linux.c inky.h
	$(CC) $(CFLAGS) -c -o $@ inky_hardware_linux.c

# Common object files
$(BUILD_DIR)/test_clear.o: test_clear.c inky.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Hardware-specific test_clear build with HARDWARE_BUILD defined
$(BUILD_DIR)/test_clear_hw.o: test_clear.c inky.h
	$(CC) $(CFLAGS) -DHARDWARE_BUILD -c -o $@ test_clear.c

# Run emulator test
test: $(EMULATOR_TARGET)
	@echo "Running emulator test..."
	$(EMULATOR_TARGET) --emulator --color 1
	@echo "Display image saved to display.ppm"
	@echo "To view: open display.ppm (or convert to PNG)"

# Run with different colors
test-colors: $(EMULATOR_TARGET)
	@echo "Testing all colors..."
	@for i in 0 1 2 3 4 5 6 7; do \
		echo "Testing color $$i..."; \
		$(EMULATOR_TARGET) --emulator --color $$i --output display_color_$$i.ppm; \
	done
	@echo "Generated display_color_*.ppm files"

# Convert PPM to PNG (requires ImageMagick)
convert-images:
	@which convert > /dev/null 2>&1 || (echo "ImageMagick not found. Install with: brew install imagemagick" && exit 1)
	@for ppm in *.ppm; do \
		if [ -f "$$ppm" ]; then \
			png="$${ppm%.ppm}.png"; \
			echo "Converting $$ppm to $$png..."; \
			convert "$$ppm" "$$png"; \
		fi \
	done

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f *.ppm *.png

# Help
help:
	@echo "Inky C Library - Makefile targets:"
	@echo ""
	@echo "  make emulator    - Build emulator version (default)"
	@echo "  make hardware    - Build hardware version (Linux only)"
	@echo "  make test        - Run emulator test with white screen"
	@echo "  make test-colors - Test all 8 colors"
	@echo "  make convert-images - Convert PPM files to PNG (requires ImageMagick)"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make help        - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./bin/test_clear_emulator --color 2  # Clear to green"
	@echo "  ./bin/test_clear_hardware --color 1  # Clear hardware to white"

.PHONY: all emulator hardware test test-colors convert-images clean help