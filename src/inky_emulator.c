#include "inky_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize with auto-detection (defaults to 5.7" for emulator)
inky_t* inky_init(bool emulator) {
    if (!emulator) {
        // Hardware initialization is in inky_hardware.c
        return NULL;
    }

    // Use common initialization with default 5.7" model
    return inky_init_common_model(INKY_IMPRESSION_5_7, true);
}

// Initialize with specific model
inky_t* inky_init_model(inky_model_t model, bool emulator) {
    if (!emulator) {
        // Hardware initialization is in inky_hardware.c
        return NULL;
    }

    // Use common initialization with specified model
    return inky_init_common_model(model, true);
}

void inky_destroy(inky_t *display) {
    if (!display) return;
    
    // Use common cleanup (no hardware-specific cleanup needed for emulator)
    inky_destroy_common(display);
}

// Stub function for display detection (returns default for emulator)
inky_model_t inky_detect_display(void) {
    return INKY_IMPRESSION_5_7;  // Default to 5.7" for emulator
}

// Stub functions for hardware operations (not used in emulator)
bool inky_hw_init_gpio(inky_t *display) { (void)display; return true; }
void inky_hw_setup(inky_t *display) { (void)display; }
void inky_hw_reset(inky_t *display) { (void)display; }
void inky_hw_send_command(inky_t *display, uint8_t command) { (void)display; (void)command; }
void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len) { (void)display; (void)data; (void)len; }
void inky_hw_busy_wait(inky_t *display) { (void)display; }
void inky_hw_update(inky_t *display) { (void)display; }
void inky_hw_set_partial_window(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { 
    (void)display; (void)x; (void)y; (void)width; (void)height; 
}
void inky_hw_partial_update(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { 
    (void)display; (void)x; (void)y; (void)width; (void)height; 
}