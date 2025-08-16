#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// Global variables for graceful shutdown
volatile bool running = true;
inky_t *display = NULL;

void signal_handler(int sig) {
    (void)sig;
    running = false;
    printf("\nShutting down...\n");
}

void button_callback(int button, void *user_data) {
    (void)user_data;
    
    const char *button_names[] = {"A", "B", "C", "D"};
    const uint8_t button_colors[] = {INKY_RED, INKY_GREEN, INKY_BLUE, INKY_YELLOW};
    
    printf("Button %s pressed! Clearing display to %s\n", 
           button_names[button], 
           (button_colors[button] == INKY_RED) ? "RED" :
           (button_colors[button] == INKY_GREEN) ? "GREEN" :
           (button_colors[button] == INKY_BLUE) ? "BLUE" : "YELLOW");
    
    if (display) {
        // Clear display to button's color
        inky_clear(display, button_colors[button]);
        
        // Add button label in corner
        uint16_t width = inky_get_width(display);
        uint16_t height = inky_get_height(display);
        
        // Draw simple pattern for each button
        switch (button) {
            case INKY_BUTTON_A:
                // Draw A pattern - triangle
                for (int y = 0; y < 50; y++) {
                    for (int x = 25 - y/2; x <= 25 + y/2 && x < width && y < height; x++) {
                        if (x >= 0) inky_set_pixel(display, x, y, INKY_BLACK);
                    }
                }
                break;
                
            case INKY_BUTTON_B:
                // Draw B pattern - rectangle
                for (int y = 0; y < 50 && y < height; y++) {
                    for (int x = 0; x < 50 && x < width; x++) {
                        if (x == 0 || x == 49 || y == 0 || y == 49 || y == 24) {
                            inky_set_pixel(display, x, y, INKY_BLACK);
                        }
                    }
                }
                break;
                
            case INKY_BUTTON_C:
                // Draw C pattern - circle
                for (int y = 0; y < 50 && y < height; y++) {
                    for (int x = 0; x < 50 && x < width; x++) {
                        int dx = x - 25;
                        int dy = y - 25;
                        int dist_sq = dx*dx + dy*dy;
                        if (dist_sq >= 400 && dist_sq <= 625) {  // Ring
                            inky_set_pixel(display, x, y, INKY_BLACK);
                        }
                    }
                }
                break;
                
            case INKY_BUTTON_D:
                // Draw D pattern - diamond
                for (int y = 0; y < 50 && y < height; y++) {
                    for (int x = 0; x < 50 && x < width; x++) {
                        int dist = abs(x - 25) + abs(y - 25);
                        if (dist == 20 || dist == 23) {
                            inky_set_pixel(display, x, y, INKY_BLACK);
                        }
                    }
                }
                break;
        }
        
        // Draw border
        for (int x = 0; x < width; x++) {
            inky_set_pixel(display, x, 0, INKY_BLACK);
            inky_set_pixel(display, x, height - 1, INKY_BLACK);
        }
        for (int y = 0; y < height; y++) {
            inky_set_pixel(display, 0, y, INKY_BLACK);
            inky_set_pixel(display, width - 1, y, INKY_BLACK);
        }
        
        // Update display
        printf("Updating display...\n");
        inky_update(display);
        
        // Save image
        char filename[64];
        snprintf(filename, sizeof(filename), "button_%c_test.ppm", 'A' + button);
        inky_emulator_save_ppm(display, filename);
    }
}

int main(void) {
    printf("Inky Button Test\n");
    printf("================\n");
    printf("This program demonstrates the button functionality.\n");
    printf("Press buttons A, B, C, or D to change the display.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize display (hardware mode)
    printf("Initializing display...\n");
    display = inky_init(false);  // Hardware mode
    if (!display) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }
    
    // Initialize buttons
    printf("Initializing buttons...\n");
    if (inky_button_init() < 0) {
        fprintf(stderr, "Failed to initialize buttons\n");
        inky_destroy(display);
        return 1;
    }
    
    // Set button callback
    inky_button_set_callback(button_callback, NULL);
    
    // Clear display to white initially
    printf("Clearing display to white...\n");
    inky_clear(display, INKY_WHITE);
    
    // Draw initial instructions
    uint16_t width = inky_get_width(display);
    uint16_t height = inky_get_height(display);
    
    // Draw simple "PRESS BUTTONS" message using basic shapes
    // P
    for (int y = height/2 - 20; y < height/2 + 20; y++) {
        inky_set_pixel(display, width/2 - 100, y, INKY_BLACK);
        if (y == height/2 - 20 || y == height/2) {
            for (int x = width/2 - 100; x < width/2 - 80; x++) {
                inky_set_pixel(display, x, y, INKY_BLACK);
            }
        }
        if (y < height/2) {
            inky_set_pixel(display, width/2 - 80, y, INKY_BLACK);
        }
    }
    
    // Draw border
    for (int x = 0; x < width; x++) {
        inky_set_pixel(display, x, 0, INKY_BLACK);
        inky_set_pixel(display, x, height - 1, INKY_BLACK);
    }
    for (int y = 0; y < height; y++) {
        inky_set_pixel(display, 0, y, INKY_BLACK);
        inky_set_pixel(display, width - 1, y, INKY_BLACK);
    }
    
    printf("Updating display...\n");
    inky_update(display);
    inky_emulator_save_ppm(display, "button_test_initial.ppm");
    
    printf("\nReady! Press buttons A, B, C, or D...\n");
    printf("Button status: ");
    
    // Main event loop
    int status_counter = 0;
    while (running) {
        // Poll for button events
        inky_button_poll();
        
        // Show button status every 100 iterations
        if (++status_counter >= 100) {
            printf("\rButton status: A:%s B:%s C:%s D:%s", 
                   inky_button_is_pressed(INKY_BUTTON_A) ? "●" : "○",
                   inky_button_is_pressed(INKY_BUTTON_B) ? "●" : "○",
                   inky_button_is_pressed(INKY_BUTTON_C) ? "●" : "○",
                   inky_button_is_pressed(INKY_BUTTON_D) ? "●" : "○");
            fflush(stdout);
            status_counter = 0;
        }
        
        // Small delay to avoid excessive CPU usage
        usleep(10000);  // 10ms
    }
    
    printf("\n\nCleaning up...\n");
    
    // Clean up
    inky_button_cleanup();
    inky_destroy(display);
    
    printf("Button test completed.\n");
    return 0;
}