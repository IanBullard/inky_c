# inky_c

C library for Inky Impression 7-color e-ink displays. Supports both Raspberry Pi hardware and cross-platform emulator.

## Scripts

```bash
script/build              # emulator backend -> build/emulator/
script/build hardware     # Raspberry Pi backend -> build/hardware/ (Linux only)
script/test               # build emulator + run CTest
./run_on_pi.sh            # build hardware library on the Pi
```

CMake is the only build system (the Makefile was removed). Backend is selected by `BUILD_EMULATOR`
(default `OFF` = hardware). Buildroot is supported as a `cmake-package` via the `install()` rules;
see README.

## Project Structure

- `include/inky.h` - Public API header
- `include/inky_internal.h` - Internal structs + `inky_hw_*` backend interface
- `src/inky_common.c` - Shared implementation
- `src/inky_hardware.c` - Raspberry Pi hardware backend (SPI/GPIO/I2C)
- `src/inky_emulator.c` - Cross-platform emulator backend (PPM output)
- `test/test_models.c` - The only test built by CMake
- `test/inky_buttons.c` - Button implementation; **not in the library build** even though `inky.h` declares the API
- `test/test_*.c` - Other example programs, not built

## Technical Details

- C99 with GNU extensions, POSIX
- Models: 5.7" 600x448 (UC8159), 13.3" 1600x1200 (EL133UF1); hardware auto-detects via EEPROM
- 7 colors + clean, 4-bit packed pixels (2 per byte)
- SPI 3 MHz, Mode 0, manual CS

## API

See `include/inky.h`. Core: `inky_init(bool emulator)`, `inky_init_model()`, `inky_clear()`,
`inky_set_pixel()`, `inky_update()`, `inky_destroy()`, `inky_emulator_save_ppm()`.

## Alpha Features

Partial updates (`inky_update_region()`) are experimental:
- 2-4 seconds vs 15-32 seconds for full refresh
- Requires full refresh every 5-6 partial updates
- Ghosting may occur

## Context

Used by `rpi0-weather` (consumes this repo via CMake `add_subdirectory`, target `inky` — keep that
name and the `BUILD_EMULATOR` option stable). Public on GitHub (based on Pimoroni's Python library).
