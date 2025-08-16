# Inky C Library

A C library implementation for the Inky Impression 5.7" (600x448) e-ink display.

## Features

- Support for Inky Impression 5.7" (600x448 resolution)
- 7-color display support (Black, White, Green, Blue, Red, Yellow, Orange)
- Hardware implementation for Raspberry Pi
- Emulator implementation for development/testing on any platform
- Efficient 4-bit packed pixel buffer
- PPM image output for emulator

## Building

### Emulator Version (All Platforms)

```bash
make emulator
```

This builds the emulator version that works on any platform and outputs PPM image files.

### Hardware Version (Raspberry Pi Only)

```bash
make hardware
```

This builds the hardware version that communicates with the actual Inky display via SPI/GPIO.
**Note:** This can only be built on Raspberry Pi with Linux ARM.

## Usage

### Clear Display Test

```bash
# Emulator - clear to white and save as PPM
./bin/test_clear_emulator --color 1

# Hardware (Raspberry Pi only) - clear actual display to white
./bin/test_clear_hardware --color 1
```

### Color Values

- 0 = BLACK
- 1 = WHITE
- 2 = GREEN
- 3 = BLUE
- 4 = RED
- 5 = YELLOW
- 6 = ORANGE
- 7 = CLEAN (white)

### Test All Colors

```bash
make test-colors
```

This generates PPM files for all 8 colors.

## API

```c
// Initialize display (emulator or hardware)
inky_t* inky_init(bool emulator);

// Clear display to a specific color
void inky_clear(inky_t *display, uint8_t color);

// Set individual pixel
void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color);

// Update display (send buffer to display)
void inky_update(inky_t *display);

// Save emulator output as PPM image
int inky_emulator_save_ppm(inky_t *display, const char *filename);

// Clean up resources
void inky_destroy(inky_t *display);
```

## Hardware Requirements

For hardware operation on Raspberry Pi:
- SPI enabled (`sudo raspi-config` -> Interface Options -> SPI)
- Connected to SPI0 (CE0)
- GPIO pins: Reset (BCM27), Busy (BCM17), DC (BCM22), CS (BCM8)

## File Structure

- `inky.h` - Main header file with API definitions
- `inky_emulator.c` - Emulator implementation (all platforms)
- `inky_hardware_linux.c` - Hardware implementation (Raspberry Pi)
- `test_clear.c` - Example program for clearing the display
- `Makefile` - Build configuration

## Notes

- Display refresh on hardware can take up to 32 seconds
- The emulator outputs PPM files which can be converted to PNG using ImageMagick
- The library uses 4-bit packed pixels for efficient memory usage (2 pixels per byte)