# Inky C Library

A C library implementation for the Inky Impression 5.7" (600x448) 7-color e-ink display. This library provides both hardware support for Raspberry Pi and an emulator for development/testing on any platform.

## Features

- **Full Color Support**: All 7 colors plus clean (Black, White, Green, Blue, Red, Yellow, Orange)
- **Dual Implementation**: 
  - Hardware driver for Raspberry Pi (Linux ARM)
  - Emulator for development on any platform
- **Efficient Memory Usage**: 4-bit packed pixel buffer (2 pixels per byte)
- **UC8159 Controller**: Full support for the UC8159 e-ink controller
- **Manual GPIO Control**: Direct control of Reset, DC, CS, and Busy pins
- **Button Support**: Full support for all 4 hardware buttons (A, B, C, D)
- **Partial Updates**: Fast region-based updates (2-4 seconds vs 15-32 seconds)
- **PPM Image Output**: Emulator saves display state as PPM images for testing

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

### Button Test Program (Raspberry Pi Only)

```bash
make buttons
```

This builds a demonstration program that shows how to use the button functionality.

### Emulated Button Test Program (All Platforms)

```bash
make emulator-buttons
```

This builds a demonstration program that shows how to use emulated button presses for testing.

### Partial Update Test Program

```bash
make partial-emulator  # All platforms
make partial-hardware  # Raspberry Pi only
```

This builds demonstration programs that show partial window update functionality for faster display updates.

## Usage

### Clear Display Test

```bash
# Emulator - clear to white and save as PPM
./bin/test_clear_emulator --color 1

# Hardware (Raspberry Pi only) - clear actual display to white
./bin/test_clear_hardware --color 1

# Button test (Raspberry Pi only) - interactive button demo
sudo ./bin/test_buttons

# Emulated button test (all platforms) - simulated button demo
./bin/test_emulator_buttons

# Partial update tests - faster region-based updates
./bin/test_partial_update_emulator --test clock   # Animated clock demo
./bin/test_partial_update_hardware --test counter # Hardware counter demo
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

### Button Usage

The library includes full support for the 4 hardware buttons:

```c
#include "inky.h"

// Button callback function
void on_button_press(int button, void *user_data) {
    printf("Button %d pressed!\n", button);
    // button values: INKY_BUTTON_A (0), INKY_BUTTON_B (1), 
    //                INKY_BUTTON_C (2), INKY_BUTTON_D (3)
}

int main() {
    // Initialize button support
    if (inky_button_init() < 0) {
        fprintf(stderr, "Failed to initialize buttons\n");
        return 1;
    }
    
    // Set callback for button presses
    inky_button_set_callback(on_button_press, NULL);
    
    // Main event loop
    while (running) {
        inky_button_poll();  // Check for button events
        
        // Check individual button states
        if (inky_button_is_pressed(INKY_BUTTON_A)) {
            printf("Button A is currently held down\n");
        }
        
        usleep(10000);  // 10ms delay
    }
    
    // Clean up
    inky_button_cleanup();
    return 0;
}
```

### Emulated Button Usage

For testing applications without hardware, you can use emulated button presses:

```c
#include "inky.h"

void button_callback(int button, void *user_data) {
    printf("Button %d pressed!\n", button);
}

int main() {
    // Initialize button support (automatically uses emulator mode on non-Linux)
    if (inky_button_init() < 0) {
        fprintf(stderr, "Failed to initialize buttons\n");
        return 1;
    }
    
    // Set callback for button presses
    inky_button_set_callback(button_callback, NULL);
    
    // Simulate button presses programmatically
    inky_button_emulate_press(INKY_BUTTON_A);  // Simulates button A press
    inky_button_emulate_press(INKY_BUTTON_B);  // Simulates button B press
    
    // Clean up
    inky_button_cleanup();
    return 0;
}
```

### Partial Update Usage

The library supports fast partial updates that only refresh specific regions of the display:

```c
#include "inky.h"

int main() {
    // Initialize display
    inky_t *display = inky_init(false);  // Hardware mode
    
    // Clear entire display (full update)
    inky_clear(display, INKY_WHITE);
    inky_update(display);  // Takes 15-32 seconds
    
    // Update only a small region (much faster!)
    for (int x = 100; x < 200; x++) {
        for (int y = 100; y < 150; y++) {
            inky_set_pixel(display, x, y, INKY_RED);
        }
    }
    
    // Partial update only the changed region
    inky_update_region(display, 100, 100, 100, 50);  // Takes 2-4 seconds!
    
    inky_destroy(display);
    return 0;
}
```

**Performance Benefits:**
- **Full Update**: 15-32 seconds, entire screen flickers
- **Partial Update**: 2-4 seconds, only updated region flickers
- **Use Cases**: Digital clocks, counters, live data, real-time applications

**Best Practices:**
- Use `inky_update()` for initial display setup or major changes
- Use `inky_update_region()` for small, frequent updates
- Ensure region coordinates are within display bounds
- Partial updates work best for regions smaller than 25% of the screen

## Public API

The library provides a clean, opaque API that hides implementation details:

```c
// Display context (opaque structure)
typedef struct inky_display inky_t;

// Initialization and cleanup
inky_t* inky_init(bool emulator);          // Initialize display
void inky_destroy(inky_t *display);        // Clean up resources

// Display operations
void inky_clear(inky_t *display, uint8_t color);                           // Clear to color
void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color); // Set pixel
uint8_t inky_get_pixel(inky_t *display, uint16_t x, uint16_t y);           // Get pixel
void inky_set_border(inky_t *display, uint8_t color);                      // Set border color
void inky_update(inky_t *display);                                         // Update display

// Utility functions
uint16_t inky_get_width(inky_t *display);   // Get display width
uint16_t inky_get_height(inky_t *display);  // Get display height

// Image output (works with both emulator and hardware)
int inky_emulator_save_ppm(inky_t *display, const char *filename);  // Save as PPM image

// Button support (hardware only - no-op on emulator)
typedef void (*inky_button_callback_t)(int button, void *user_data);  // Button callback type
int inky_button_init(void);                                           // Initialize buttons
void inky_button_set_callback(inky_button_callback_t callback, void *user_data);  // Set callback
void inky_button_poll(void);                                          // Poll for events
bool inky_button_is_pressed(int button);                             // Check button state
void inky_button_cleanup(void);                                       // Clean up buttons
void inky_button_emulate_press(int button);                               // Emulate button press (emulator only)

// Partial update functions - faster than full updates
void inky_update_region(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height);  // Update specific region
```

## Hardware Requirements

### Raspberry Pi Setup

1. **Enable SPI Interface**:
   ```bash
   sudo raspi-config
   # Navigate to: Interface Options -> SPI -> Enable
   ```

2. **Pin Connections** (BCM numbering):
   - **SPI0 MOSI**: GPIO 10 (Physical Pin 19)
   - **SPI0 SCLK**: GPIO 11 (Physical Pin 23)
   - **Reset**: GPIO 27 (Physical Pin 13)
   - **Busy**: GPIO 17 (Physical Pin 11)
   - **DC**: GPIO 22 (Physical Pin 15)
   - **CS**: GPIO 8/CE0 (Physical Pin 24) - Manually controlled
   - **Button A**: GPIO 5 (Physical Pin 29)
   - **Button B**: GPIO 6 (Physical Pin 31)
   - **Button C**: GPIO 16 (Physical Pin 36)
   - **Button D**: GPIO 24 (Physical Pin 18)

3. **Required Permissions**:
   - Run with `sudo` for GPIO/SPI access
   - Or add user to `gpio` and `spi` groups:
     ```bash
     sudo usermod -a -G gpio,spi $USER
     # Logout and login for changes to take effect
     ```

## File Structure

```
inky_c/
├── inky.h                  # Public API header
├── inky_internal.h         # Internal implementation header  
├── inky_common.c           # Shared implementation (buffer operations, etc.)
├── inky_emulator.c         # Emulator-specific code (PPM output, stubs)
├── inky_hardware.c         # Hardware-specific code (SPI, GPIO, UC8159)
├── inky_buttons.c          # Button support (GPIO input, callbacks)
├── test_clear.c            # Example: Clear display test program
├── test_buttons.c          # Example: Interactive button demonstration
├── Makefile                # Build configuration
├── run_on_pi.sh           # Helper script for Raspberry Pi
└── README.md              # This documentation
```

### API Design

The library uses a **clean separation of concerns**:

- **`inky.h`**: Clean public API with opaque pointers - only what users need
- **`inky_internal.h`**: Internal structure and function declarations
- **`inky_common.c`**: Shared code (buffer operations, pixel manipulation, common init/destroy)
- **`inky_emulator.c`**: Emulator-specific code (PPM generation, hardware stubs)
- **`inky_hardware.c`**: Hardware-specific code (SPI/GPIO communication, UC8159 commands)
- **`inky_buttons.c`**: Button support (GPIO input handling, event callbacks)

This design ensures:
- **Maximum code reuse** between emulator and hardware
- **Clean public API** with no implementation details exposed
- **Easy testing** - emulator behavior closely matches hardware

## Implementation Details

### Buffer Format
- **Resolution**: 600x448 pixels
- **Color Depth**: 4 bits per pixel (8 colors)
- **Packing**: 2 pixels per byte (high nibble = even pixel, low nibble = odd pixel)
- **Buffer Size**: 134,400 bytes (600 × 448 ÷ 2)

### SPI Communication
- **Speed**: 3 MHz
- **Mode**: 0 (CPOL=0, CPHA=0)
- **Chip Select**: Manual control via GPIO (SPI_NO_CS mode)
- **Chunk Size**: 4096 bytes for data transmission

### Display Update Sequence
1. Send display data (UC8159_DTM1)
2. Power on (UC8159_PON)
3. Display refresh (UC8159_DRF) - waits for busy signal
4. Power off (UC8159_POF)

## Troubleshooting

### Common Issues

1. **"Failed to open SPI device"**
   - Ensure SPI is enabled: `sudo raspi-config`
   - Check device exists: `ls /dev/spidev*`

2. **"Failed to request GPIO lines"**
   - Run with sudo or add user to gpio group
   - Check GPIO chip exists: `ls /dev/gpiochip*`

3. **Display doesn't update**
   - Verify all connections are correct
   - Check busy pin is properly connected
   - Ensure display is powered
   - Wait up to 32 seconds for refresh

4. **Build fails on non-Linux**
   - Use `make emulator` instead of `make hardware`
   - Hardware version only builds on Raspberry Pi

## Notes

- **Display Refresh Time**: 
  - Full update: 15-32 seconds depending on content
  - Partial update: 2-4 seconds for small regions
- **Power Consumption**: Display only draws power during refresh
- **Partial Update Limitations**: Best for regions < 25% of screen, uses UC8159 commands 0x90/0x91/0x92
- **Image Formats**: Emulator outputs PPM files, convert to PNG with ImageMagick:
  ```bash
  convert display.ppm display.png
  ```
- **Memory Efficiency**: Uses packed 4-bit pixels to minimize memory usage
- **Version History**: 
  - `first_success`: Initial working implementation
  - `v1.0`: Complete library with button support

## License

Based on Pimoroni's Inky library for Python. See original repository for license details.