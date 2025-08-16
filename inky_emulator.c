#include "inky_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inky_t* inky_init(bool emulator) {
    if (!emulator) {
        // Hardware initialization is in inky_hardware.c
        return NULL;
    }
    
    // Use common initialization
    return inky_init_common(true);
}

void inky_destroy(inky_t *display) {
    if (!display) return;
    
    // Use common cleanup (no hardware-specific cleanup needed for emulator)
    inky_destroy_common(display);
}

// Stub functions for hardware operations (not used in emulator)
bool inky_hw_init_gpio(inky_t *display) { (void)display; return true; }
void inky_hw_setup(inky_t *display) { (void)display; }
void inky_hw_reset(inky_t *display) { (void)display; }
void inky_hw_send_command(inky_t *display, uint8_t command) { (void)display; (void)command; }
void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len) { (void)display; (void)data; (void)len; }
void inky_hw_busy_wait(inky_t *display) { (void)display; }
void inky_hw_update(inky_t *display) { (void)display; }