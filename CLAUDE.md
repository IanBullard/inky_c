# inky_c

C library for Inky Impression 7-color e-ink displays. Supports both Raspberry Pi hardware and cross-platform emulator.

## Quick Reference

```bash
# Build
make

# Run tests
make test

# Deploy to Pi
./run_on_pi.sh
```

## Project Structure

- `include/inky.h` - Public API header
- `src/inky_common.c` - Shared implementation
- `src/inky_hardware.c` - Raspberry Pi hardware driver (SPI/GPIO)
- `src/inky_emulator.c` - Cross-platform emulator (PPM output)
- `src/inky_buttons.c` - Button handling with callbacks
- `test/` - Test programs

## Technical Details

- Pure C (C99, POSIX)
- Display: 600x448, 7 colors (Black, White, Green, Blue, Red, Yellow, Orange)
- 4-bit packed pixels (2 pixels per byte)
- UC8159 e-ink controller
- SPI communication (3 MHz, Mode 0)

## API Overview

```c
inky_t* inky_create(void);
void inky_destroy(inky_t* inky);
void inky_clear(inky_t* inky, inky_color_t color);
void inky_set_pixel(inky_t* inky, int x, int y, inky_color_t color);
void inky_show(inky_t* inky);

// Buttons
void inky_set_button_callback(inky_t* inky, inky_button_t button, inky_button_callback_t cb);
void inky_poll_buttons(inky_t* inky);
```

## Alpha Features

Partial updates are experimental:
- 2-4 seconds vs 15-32 seconds for full refresh
- Requires full refresh every 5-6 partial updates
- Ghosting may occur

## Context

This library is used by `rpi0-weather` project. Public on GitHub (based on Pimoroni's Python library).
