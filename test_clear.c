#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s [--emulator|--hardware] [--color COLOR]\n", prog_name);
    printf("Options:\n");
    printf("  --emulator    Use emulator mode (default)\n");
    printf("  --hardware    Use hardware mode\n");
    printf("  --color COLOR Set clear color (0-7):\n");
    printf("                0=BLACK, 1=WHITE, 2=GREEN, 3=BLUE\n");
    printf("                4=RED, 5=YELLOW, 6=ORANGE, 7=CLEAN\n");
    printf("                Default: WHITE\n");
    printf("  --output FILE Save emulator output to FILE (default: display.ppm)\n");
}

int main(int argc, char *argv[]) {
    bool use_emulator = true;
    uint8_t clear_color = INKY_WHITE;
    const char *output_file = "display.ppm";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--emulator") == 0) {
            use_emulator = true;
        } else if (strcmp(argv[i], "--hardware") == 0) {
            use_emulator = false;
        } else if (strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
            clear_color = atoi(argv[++i]);
            if (clear_color > 7) {
                fprintf(stderr, "Error: Color must be 0-7\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    const char *color_names[] = {
        "BLACK", "WHITE", "GREEN", "BLUE", 
        "RED", "YELLOW", "ORANGE", "CLEAN"
    };
    
    printf("Inky Display Clear Test\n");
    printf("========================\n");
    printf("Mode: %s\n", use_emulator ? "Emulator" : "Hardware");
    printf("Clear color: %s (%d)\n", color_names[clear_color], clear_color);
    
    if (use_emulator) {
        printf("Output file: %s\n", output_file);
    }
    
    printf("\n");
    
    // Initialize display
    printf("Initializing display...\n");
    inky_t *display = inky_init(use_emulator);
    if (!display) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }
    
    // Clear the display
    printf("Clearing display to %s...\n", color_names[clear_color]);
    inky_clear(display, clear_color);
    
    // Draw a simple test pattern - add a few pixels of different colors
    if (clear_color == INKY_WHITE) {
        // Draw a small test pattern in the corner
        printf("Adding test pattern...\n");
        
        // Draw a small square of each color
        for (int c = 0; c < 7; c++) {
            int x_offset = 10 + (c * 20);
            int y_offset = 10;
            
            for (int y = 0; y < 15; y++) {
                for (int x = 0; x < 15; x++) {
                    inky_set_pixel(display, x_offset + x, y_offset + y, c);
                }
            }
        }
        
        // Draw a border around the display
        printf("Drawing border...\n");
        for (int x = 0; x < display->width; x++) {
            inky_set_pixel(display, x, 0, INKY_BLACK);
            inky_set_pixel(display, x, display->height - 1, INKY_BLACK);
        }
        for (int y = 0; y < display->height; y++) {
            inky_set_pixel(display, 0, y, INKY_BLACK);
            inky_set_pixel(display, display->width - 1, y, INKY_BLACK);
        }
    }
    
    // Update the display
    printf("Updating display...\n");
    inky_update(display);
    
    // Save to file if using emulator
    if (use_emulator) {
        printf("Saving image to %s...\n", output_file);
        if (inky_emulator_save_ppm(display, output_file) < 0) {
            fprintf(stderr, "Failed to save image\n");
            inky_destroy(display);
            return 1;
        }
    } else {
        printf("Display update sent to hardware.\n");
        printf("Note: The display refresh can take up to 32 seconds.\n");
    }
    
    // Clean up
    printf("Cleaning up...\n");
    inky_destroy(display);
    
    printf("Done!\n");
    return 0;
}