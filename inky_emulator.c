#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Color palette - RGB values for each color
static const uint8_t color_palette[8][3] = {
    {57, 48, 57},       // BLACK
    {255, 255, 255},    // WHITE
    {58, 91, 70},       // GREEN
    {61, 59, 94},       // BLUE
    {156, 72, 75},      // RED
    {208, 190, 71},     // YELLOW
    {177, 106, 73},     // ORANGE
    {255, 255, 255}     // CLEAN (white)
};

inky_t* inky_init(bool emulator) {
    if (!emulator) {
        // Hardware initialization is in inky_hardware.c
        return NULL;
    }
    
    inky_t *display = calloc(1, sizeof(inky_t));
    if (!display) {
        return NULL;
    }
    
    display->width = INKY_WIDTH;
    display->height = INKY_HEIGHT;
    display->is_emulator = true;
    display->border_color = INKY_WHITE;
    display->h_flip = false;
    display->v_flip = false;
    
    // Calculate buffer size - 4 bits per pixel, packed
    display->buffer_size = (display->width * display->height + 1) / 2;
    display->buffer = calloc(display->buffer_size, 1);
    
    if (!display->buffer) {
        free(display);
        return NULL;
    }
    
    // Initialize to white
    memset(display->buffer, 0x11, display->buffer_size);  // 0x11 = WHITE|WHITE (two pixels)
    
    return display;
}

void inky_destroy(inky_t *display) {
    if (!display) return;
    
    if (display->buffer) {
        free(display->buffer);
    }
    
    free(display);
}

void inky_clear(inky_t *display, uint8_t color) {
    if (!display || !display->buffer) return;
    
    // Pack two pixels of the same color into one byte
    uint8_t packed_color = ((color & 0x0F) << 4) | (color & 0x0F);
    memset(display->buffer, packed_color, display->buffer_size);
}

void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color) {
    if (!display || !display->buffer) return;
    if (x >= display->width || y >= display->height) return;
    
    size_t pixel_index = y * display->width + x;
    size_t byte_index = pixel_index / 2;
    
    if (pixel_index & 1) {
        // Odd pixel - low nibble
        display->buffer[byte_index] = (display->buffer[byte_index] & 0xF0) | (color & 0x0F);
    } else {
        // Even pixel - high nibble
        display->buffer[byte_index] = (display->buffer[byte_index] & 0x0F) | ((color & 0x0F) << 4);
    }
}

uint8_t inky_get_pixel(inky_t *display, uint16_t x, uint16_t y) {
    if (!display || !display->buffer) return 0;
    if (x >= display->width || y >= display->height) return 0;
    
    size_t pixel_index = y * display->width + x;
    size_t byte_index = pixel_index / 2;
    
    if (pixel_index & 1) {
        // Odd pixel - low nibble
        return display->buffer[byte_index] & 0x0F;
    } else {
        // Even pixel - high nibble
        return (display->buffer[byte_index] >> 4) & 0x0F;
    }
}

void inky_set_border(inky_t *display, uint8_t color) {
    if (!display) return;
    display->border_color = color & 0x07;
}

void inky_update(inky_t *display) {
    if (!display) return;
    
    if (display->is_emulator) {
        printf("Emulator: Display updated (use inky_emulator_save_ppm to save image)\n");
    }
}

int inky_emulator_save_ppm(inky_t *display, const char *filename) {
    if (!display || !display->is_emulator || !filename) return -1;
    
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Failed to open file");
        return -1;
    }
    
    // Write PPM header
    fprintf(fp, "P6\n%d %d\n255\n", display->width, display->height);
    
    // Write pixel data
    for (uint16_t y = 0; y < display->height; y++) {
        for (uint16_t x = 0; x < display->width; x++) {
            uint8_t color = inky_get_pixel(display, x, y);
            if (color > 7) color = 7;  // Clamp to valid range
            
            // Write RGB values from palette
            fwrite(color_palette[color], 1, 3, fp);
        }
    }
    
    fclose(fp);
    printf("Saved display image to %s\n", filename);
    return 0;
}

// Stub functions for hardware operations (not used in emulator)
void inky_hw_setup(inky_t *display) { (void)display; }
void inky_hw_reset(inky_t *display) { (void)display; }
void inky_hw_send_command(inky_t *display, uint8_t command) { (void)display; (void)command; }
void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len) { (void)display; (void)data; (void)len; }
void inky_hw_busy_wait(inky_t *display) { (void)display; }