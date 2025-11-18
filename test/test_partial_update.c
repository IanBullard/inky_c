#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s [--emulator|--hardware] [options]\n", prog_name);
    printf("Options:\n");
#ifdef HARDWARE_BUILD
    printf("  --emulator    Use emulator mode\n");
    printf("  --hardware    Use hardware mode (default)\n");
#else
    printf("  --emulator    Use emulator mode (default)\n");
    printf("  --hardware    Use hardware mode\n");
#endif
    printf("  --test TYPE   Test type:\n");
    printf("                clock    - Animated digital clock\n");
    printf("                counter  - Simple counter\n");
    printf("                corner   - Update corners sequentially\n");
    printf("                random   - Random region updates\n");
    printf("                Default: clock\n");
    printf("  --output FILE Save emulator output to FILE (default: partial_test.ppm)\n");
}

// Draw a digit at specified position
void draw_digit(inky_t *display, int digit, int x_offset, int y_offset) {
    // Simple 7-segment style digit patterns (15x25 pixels each)
    static const uint8_t digit_patterns[10][25][15] = {
        // 0
        {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
         {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}},
        // 1
        {{0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
         {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}},
        // 2-9 simplified for brevity - just use filled rectangles with different colors
        {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}},
        {{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}},
        {{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}, {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}},
        {{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}},
        {{5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}, {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}},
        {{6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}, {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6}},
        {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}}
    };
    
    if (digit < 0 || digit > 9) return;
    
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 15; x++) {
            int px = x_offset + x;
            int py = y_offset + y;
            if (px < INKY_WIDTH && py < INKY_HEIGHT) {
                inky_set_pixel(display, px, py, digit_patterns[digit][y][x]);
            }
        }
    }
}

// Draw current time as HH:MM
void draw_time(inky_t *display, int hours, int minutes) {
    // Clear the time display area
    for (int y = 200; y < 235; y++) {
        for (int x = 200; x < 400; x++) {
            inky_set_pixel(display, x, y, INKY_WHITE);
        }
    }
    
    // Draw digits: HH:MM (15px width + 5px spacing each, colon = 5px)
    int x_pos = 220;
    draw_digit(display, hours / 10, x_pos, 205);
    x_pos += 20;
    draw_digit(display, hours % 10, x_pos, 205);
    x_pos += 20;
    
    // Draw colon
    inky_set_pixel(display, x_pos + 2, 212, INKY_BLACK);
    inky_set_pixel(display, x_pos + 2, 218, INKY_BLACK);
    x_pos += 10;
    
    draw_digit(display, minutes / 10, x_pos, 205);
    x_pos += 20;
    draw_digit(display, minutes % 10, x_pos, 205);
}

int test_clock(inky_t *display, bool use_emulator, const char *output_file) {
    printf("Running animated clock test (partial updates)...\n");
    
    // Clear display
    inky_clear(display, INKY_WHITE);
    
    // Draw static elements
    for (int x = 0; x < inky_get_width(display); x++) {
        inky_set_pixel(display, x, 0, INKY_BLACK);
        inky_set_pixel(display, x, inky_get_height(display) - 1, INKY_BLACK);
    }
    for (int y = 0; y < inky_get_height(display); y++) {
        inky_set_pixel(display, 0, y, INKY_BLACK);
        inky_set_pixel(display, inky_get_width(display) - 1, y, INKY_BLACK);
    }
    
    // Title
    const char *title = "PARTIAL UPDATE CLOCK TEST";
    for (int i = 0; title[i] && i < 25; i++) {
        for (int y = 0; y < 10; y++) {
            for (int x = 0; x < 8; x++) {
                inky_set_pixel(display, 50 + i * 10 + x, 50 + y, INKY_RED);
            }
        }
    }
    
    // Initial full update
    printf("Initial full display update...\n");
    inky_update(display);
    if (use_emulator) {
        inky_emulator_save_ppm(display, output_file);
    }
    
    // Simulate time updates (1 second intervals in real clock)
    time_t start_time = time(NULL);
    struct tm *tm_info = localtime(&start_time);
    
    for (int iteration = 0; iteration < 10; iteration++) {
        // Calculate time to display
        int hours = (tm_info->tm_hour + iteration / 60) % 24;
        int minutes = (tm_info->tm_min + iteration) % 60;
        
        printf("Updating time to %02d:%02d (iteration %d)\n", hours, minutes, iteration + 1);
        
        // Update just the time region
        draw_time(display, hours, minutes);
        
        // Partial update only the time region (200,200) to (400,235)
        printf("Performing partial update...\n");
        inky_update_region(display, 200, 200, 200, 35);
        
        if (use_emulator) {
            char filename[64];
            snprintf(filename, sizeof(filename), "clock_%02d_%02d.ppm", hours, minutes);
            inky_emulator_save_ppm(display, filename);
        }
        
        // In real hardware, this would be 60 seconds; for demo, use shorter delay
        if (!use_emulator) {
            printf("Waiting for next update (hardware mode)...\n");
            sleep(5);  // 5 seconds between updates for demo
        }
    }
    
    return 0;
}

int test_counter(inky_t *display, bool use_emulator, const char *output_file) {
    printf("Running counter test (partial updates)...\n");
    
    // Clear display
    inky_clear(display, INKY_WHITE);
    
    // Initial full update
    inky_update(display);
    if (use_emulator) {
        inky_emulator_save_ppm(display, output_file);
    }
    
    // Counter in center
    for (int count = 0; count < 20; count++) {
        // Clear counter area
        for (int y = 200; y < 250; y++) {
            for (int x = 250; x < 350; x++) {
                inky_set_pixel(display, x, y, INKY_WHITE);
            }
        }
        
        // Draw counter value
        uint8_t color = (count % 7) + 1;  // Cycle through colors
        for (int y = 0; y < 40; y++) {
            for (int x = 0; x < 80; x++) {
                if ((x + y) % 4 == 0) {  // Simple pattern
                    inky_set_pixel(display, 260 + x, 205 + y, color);
                }
            }
        }
        
        printf("Counter: %d\n", count);
        
        // Partial update only the counter region
        inky_update_region(display, 250, 200, 100, 50);
        
        if (use_emulator) {
            char filename[64];
            snprintf(filename, sizeof(filename), "counter_%02d.ppm", count);
            inky_emulator_save_ppm(display, filename);
        }
        
        if (!use_emulator) {
            sleep(1);  // 1 second between updates
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    // Default behavior based on build type
#ifdef HARDWARE_BUILD
    bool use_emulator = false;
#else
    bool use_emulator = true;
#endif
    const char *test_type = "clock";
    const char *output_file = "partial_test.ppm";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--emulator") == 0) {
            use_emulator = true;
        } else if (strcmp(argv[i], "--hardware") == 0) {
            use_emulator = false;
        } else if (strcmp(argv[i], "--test") == 0 && i + 1 < argc) {
            test_type = argv[++i];
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
    
    printf("Inky Display Partial Update Test\n");
    printf("=================================\n");
    printf("Mode: %s\n", use_emulator ? "Emulator" : "Hardware");
    printf("Test: %s\n", test_type);
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
    
    int result = 0;
    
    // Run selected test
    if (strcmp(test_type, "clock") == 0) {
        result = test_clock(display, use_emulator, output_file);
    } else if (strcmp(test_type, "counter") == 0) {
        result = test_counter(display, use_emulator, output_file);
    } else {
        fprintf(stderr, "Unknown test type: %s\n", test_type);
        result = 1;
    }
    
    // Clean up
    printf("Cleaning up...\n");
    inky_destroy(display);
    
    printf("Partial update test completed!\n");
    return result;
}