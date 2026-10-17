# Inky C Library v1.1-alpha

> ⚠️ **ALPHA RELEASE WARNING**: This version includes partial update functionality which is complex and has significant limitations. New users should start with full display updates before attempting partial updates. See "Advanced Features (Alpha)" below.

A C library for Pimoroni Inky Impression 7-color e-ink displays. It provides a hardware backend for Raspberry Pi and an emulator backend for development and testing on any platform.

## Supported Displays

| Model | Resolution | Controller | `inky_model_t` |
|-------|------------|------------|----------------|
| Inky Impression 5.7" | 600x448 | UC8159 | `INKY_IMPRESSION_5_7` |
| Inky Impression 13.3" | 1600x1200 | EL133UF1 | `INKY_IMPRESSION_13_3` |

On hardware, `inky_init()` detects the model from the display's EEPROM. The emulator defaults to the 5.7".

## Features

- **7 colors plus clean**: Black, White, Green, Blue, Red, Yellow, Orange, Clean
- **Two backends**, selected at build time:
  - Hardware: Raspberry Pi over SPI/GPIO/I2C (Linux only)
  - Emulator: any platform, saves the display buffer as PPM images
- **Packed buffer**: 4 bits per pixel, 2 pixels per byte
- **Manual GPIO control**: Reset, DC, CS, and Busy pins
- **[Alpha] Partial updates**: region updates in 2-4 seconds instead of 15-32

## Building

The library builds with CMake (3.21+). The backend is chosen with the `BUILD_EMULATOR` option (default `OFF` = hardware).

```bash
script/build              # emulator backend -> build/emulator/libinky.a
script/build hardware     # hardware backend -> build/hardware/libinky.a (Raspberry Pi only)
script/test               # build emulator and run tests (images land in build/emulator/)
```

Or with CMake directly:

```bash
cmake -S . -B build -DBUILD_EMULATOR=ON
cmake --build build
```

| Option | Default | Effect |
|--------|---------|--------|
| `BUILD_EMULATOR` | `OFF` | `ON` builds the emulator backend; `OFF` builds the Raspberry Pi hardware backend |
| `INKY_BUILD_TESTS` | `ON` when top-level | Builds test programs and registers them with CTest (emulator only) |

### Installing

```bash
cmake --install build --prefix /usr/local
```

This installs `lib/libinky.a` and `include/inky.h`.

### Using from another CMake project

```cmake
add_subdirectory(inky_c)
target_link_libraries(my_app PRIVATE inky)
```

Set `BUILD_EMULATOR` before the `add_subdirectory` call to select the backend.

### Buildroot

The library builds as a Buildroot `cmake-package`; Buildroot supplies the cross toolchain. A minimal `package/inky-c/inky-c.mk`:

```make
INKY_C_VERSION = <commit-sha>
INKY_C_SITE = $(call github,IanBullard,inky_c,$(INKY_C_VERSION))
INKY_C_LICENSE = MIT
INKY_C_LICENSE_FILES = LICENSE
INKY_C_INSTALL_STAGING = YES
INKY_C_INSTALL_TARGET = NO

$(eval $(cmake-package))
```

It is a static library, so it installs to staging only; packages that depend on it link it into their own binaries. You also need a matching `Config.in` entry (`BR2_PACKAGE_INKY_C`).

## Quick Start

```c
#include "inky.h"

int main(void) {
    inky_t *display = inky_init(true);  // true = emulator mode

    inky_clear(display, INKY_WHITE);

    for (int y = 100; y < 200; y++) {
        for (int x = 100; x < 300; x++) {
            inky_set_pixel(display, x, y, INKY_RED);
        }
    }

    inky_update(display);  // 15-32 seconds on hardware

    inky_emulator_save_ppm(display, "my_display.ppm");

    inky_destroy(display);
    return 0;
}
```

The `emulator` argument must match the backend the library was built with.

### Color Values

| Value | Name |
|-------|------|
| 0 | `INKY_BLACK` |
| 1 | `INKY_WHITE` |
| 2 | `INKY_GREEN` |
| 3 | `INKY_BLUE` |
| 4 | `INKY_RED` |
| 5 | `INKY_YELLOW` |
| 6 | `INKY_ORANGE` |
| 7 | `INKY_CLEAN` (white) |

## Public API

The full API is in `include/inky.h`. The display context is opaque.

```c
typedef struct inky_display inky_t;

// Initialization and cleanup
inky_t* inky_init(bool emulator);                              // Auto-detect model
inky_t* inky_init_model(inky_model_t model, bool emulator);    // Explicit model
void inky_destroy(inky_t *display);

// Drawing
void inky_clear(inky_t *display, uint8_t color);
void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color);
uint8_t inky_get_pixel(inky_t *display, uint16_t x, uint16_t y);
void inky_set_border(inky_t *display, uint8_t color);

// Display refresh
void inky_update(inky_t *display);

// Dimensions
uint16_t inky_get_width(inky_t *display);
uint16_t inky_get_height(inky_t *display);

// Save buffer as PPM (works with both backends)
int inky_emulator_save_ppm(inky_t *display, const char *filename);

// [ALPHA] Partial updates
void inky_update_region(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
bool inky_should_full_refresh(inky_t *display);
int inky_get_partial_count(inky_t *display);
```

### Buttons

`inky.h` declares a button API (`inky_button_init`, `inky_button_set_callback`, `inky_button_poll`, `inky_button_is_pressed`, `inky_button_cleanup`, `inky_button_emulate_press`). **Its implementation, `test/inky_buttons.c`, is not part of the library build.** To use buttons today, compile that file into your application.

## Project Layout

```
inky_c/
├── include/
│   ├── inky.h            # Public API
│   └── inky_internal.h   # Internal structures and backend interface
├── src/
│   ├── inky_common.c     # Shared code: buffer, pixels, update logic
│   ├── inky_emulator.c   # Emulator backend (PPM output, hardware stubs)
│   └── inky_hardware.c   # Hardware backend (SPI, GPIO, EEPROM detection)
├── test/
│   ├── test_models.c     # Built and run by CTest
│   ├── inky_buttons.c    # Button implementation (not yet in the library)
│   └── test_*.c          # Other example programs (not built by CMake)
├── script/
│   ├── build             # Configure and build a backend
│   └── test              # Build emulator and run tests
├── CMakeLists.txt
└── run_on_pi.sh          # Build the hardware library on a Pi
```

The backends implement the `inky_hw_*` functions declared in `inky_internal.h`; `inky_common.c` calls them and contains everything else.

## Hardware Setup (Raspberry Pi)

1. **Enable SPI and I2C**:
   ```bash
   sudo raspi-config
   # Interface Options -> SPI -> Enable
   # Interface Options -> I2C -> Enable   (used for display detection)
   ```

2. **Pin connections** (BCM numbering):
   - **SPI0 MOSI**: GPIO 10 (Physical Pin 19)
   - **SPI0 SCLK**: GPIO 11 (Physical Pin 23)
   - **Reset**: GPIO 27 (Physical Pin 13)
   - **Busy**: GPIO 17 (Physical Pin 11)
   - **DC**: GPIO 22 (Physical Pin 15)
   - **CS**: GPIO 8/CE0 (Physical Pin 24), manually controlled
   - **Button A**: GPIO 5 (Physical Pin 29)
   - **Button B**: GPIO 6 (Physical Pin 31)
   - **Button C**: GPIO 16 (Physical Pin 36)
   - **Button D**: GPIO 24 (Physical Pin 18)

3. **Permissions**: run with `sudo`, or add the user to the `gpio`, `spi`, and `i2c` groups:
   ```bash
   sudo usermod -a -G gpio,spi,i2c $USER
   # Log out and back in
   ```

## Implementation Details

### Buffer Format
- **Color depth**: 4 bits per pixel
- **Packing**: 2 pixels per byte (high nibble = even pixel, low nibble = odd pixel)
- **Buffer size**: width × height ÷ 2 (134,400 bytes for the 5.7")

### SPI Communication
- **Device**: `/dev/spidev0.0`
- **Speed**: 3 MHz
- **Mode**: 0 (CPOL=0, CPHA=0)
- **Chip select**: manual GPIO control (SPI_NO_CS mode)
- **Chunk size**: 4096 bytes

### Display Update Sequence (UC8159)
1. Send display data (DTM1)
2. Power on (PON)
3. Display refresh (DRF), wait for busy
4. Power off (POF)

## Troubleshooting

1. **"Failed to open SPI device"**
   - Enable SPI: `sudo raspi-config`
   - Check the device exists: `ls /dev/spidev*`

2. **"Failed to request GPIO lines"**
   - Run with sudo or add the user to the `gpio` group
   - Check the GPIO chip exists: `ls /dev/gpiochip*`

3. **Display doesn't update**
   - Verify connections, especially the busy pin
   - Ensure the display is powered
   - Wait up to 32 seconds for a full refresh

4. **Hardware build fails on macOS/Windows**
   - The hardware backend needs Linux SPI/GPIO/I2C headers. Use `script/build` (emulator) instead.

## Advanced Features (Alpha): Partial Updates

> ⚠️ **WARNING**: Partial updates are easy to misuse. Most applications should use full updates only.

### When to Use Them
- **Real-time content**: clocks, counters, status indicators
- **Simple content only**: text, basic shapes, single-color regions

### When Not To
- **Complex graphics**: photos, detailed images, gradients
- **Large color changes**: high-contrast transitions, big solid areas
- **While learning the library**

| Content Type | Method | Reason |
|-------------|--------|--------|
| Text/digits changing | `inky_update_region()` | Small, simple area |
| Clock display | `inky_update_region()` | Real-time requirement |
| Status indicators | `inky_update_region()` | Small, frequent changes |
| Photos/images | `inky_update()` | Complex content causes ghosting |
| Color changes > 25% of screen | `inky_update()` | Approaches full update time |

### Usage

```c
inky_t *display = inky_init(false);  // Hardware mode

inky_clear(display, INKY_WHITE);
inky_update(display);  // 15-32 seconds

for (int x = 100; x < 200; x++) {
    for (int y = 100; y < 150; y++) {
        inky_set_pixel(display, x, y, INKY_RED);
    }
}
inky_update_region(display, 100, 100, 100, 50);  // 2-4 seconds
```

Managing ghosting:

```c
if (inky_should_full_refresh(display)) {
    inky_update(display);                     // Clears ghosting, resets count
} else {
    inky_update_region(display, x, y, w, h);
}
```

### Limitations
- **Ghosting**: faint ghost images appear after 5-6 partial updates. The library prints a warning from the 5th; `inky_update()` clears it.
- **Content**: best for regions under 25% of the screen with simple content.
- **Boundaries**: visible edges around the updated region are normal e-ink behavior.
- **Commands**: uses UC8159 partial window commands 0x90/0x91/0x92.

## Notes

- **Refresh time**: full update 15-32 seconds; partial update 2-4 seconds for small regions
- **Power**: the display only draws power during refresh
- **PPM to PNG**: `convert display.ppm display.png` (ImageMagick)

## License

MIT; see `LICENSE`. Based on Pimoroni's Inky library for Python.
