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
    
    printf("*** BUTTON %s PRESSED! ***\n", button_names[button]);
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
    
    // Skip display initialization for faster button testing
    display = NULL;
    
    // Initialize buttons
    printf("Initializing buttons...\n");
    if (inky_button_init() < 0) {
        fprintf(stderr, "Failed to initialize buttons\n");
        return 1;
    }
    
    // Set button callback
    inky_button_set_callback(button_callback, NULL);
    
    // No display operations - focus on button testing only
    
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
    
    printf("Button test completed.\n");
    return 0;
}