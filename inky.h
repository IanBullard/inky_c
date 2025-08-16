#ifndef INKY_H
#define INKY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Display dimensions for Inky Impression 5.7"
#define INKY_WIDTH  600
#define INKY_HEIGHT 448

// Color definitions
#define INKY_BLACK  0
#define INKY_WHITE  1
#define INKY_GREEN  2
#define INKY_BLUE   3
#define INKY_RED    4
#define INKY_YELLOW 5
#define INKY_ORANGE 6
#define INKY_CLEAN  7

// Button definitions
#define INKY_BUTTON_A 0
#define INKY_BUTTON_B 1
#define INKY_BUTTON_C 2
#define INKY_BUTTON_D 3

// Opaque display context - implementation details hidden
typedef struct inky_display inky_t;

// Initialize display (emulator or hardware based on parameter)
inky_t* inky_init(bool emulator);

// Clean up and free resources
void inky_destroy(inky_t *display);

// Clear entire display to specified color
void inky_clear(inky_t *display, uint8_t color);

// Set a single pixel
void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color);

// Get a single pixel color
uint8_t inky_get_pixel(inky_t *display, uint16_t x, uint16_t y);

// Set the border color (displayed around active area)
void inky_set_border(inky_t *display, uint8_t color);

// Update the physical display with buffer contents
void inky_update(inky_t *display);

// Save current display buffer as PPM image (works with both emulator and hardware)
// Returns 0 on success, -1 on error
int inky_emulator_save_ppm(inky_t *display, const char *filename);

// Get display dimensions
uint16_t inky_get_width(inky_t *display);
uint16_t inky_get_height(inky_t *display);

// Button support (hardware only - no-op on emulator)
// Callback function type for button presses
typedef void (*inky_button_callback_t)(int button, void *user_data);

// Initialize button GPIO (call once at startup)
int inky_button_init(void);

// Set callback function for button presses
void inky_button_set_callback(inky_button_callback_t callback, void *user_data);

// Poll for button events (call regularly in main loop)
void inky_button_poll(void);

// Check if a specific button is currently pressed
bool inky_button_is_pressed(int button);

// Clean up button resources
void inky_button_cleanup(void);

#endif // INKY_H