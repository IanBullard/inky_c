#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile bool running = true;

void signal_handler(int sig) {
    (void)sig;
    running = false;
    printf("\nShutting down...\n");
}

void button_callback(int button, void *user_data) {
    (void)user_data;
    
    const char *button_names[] = {"A", "B", "C", "D"};
    printf("*** EMULATED BUTTON %s PRESSED! ***\n", button_names[button]);
}

int main(void) {
    printf("Inky Emulated Button Test\n");
    printf("=========================\n");
    printf("This program demonstrates emulated button presses.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize button support (will automatically use emulator mode on non-Linux)
    printf("Initializing button support...\n");
    if (inky_button_init() < 0) {
        fprintf(stderr, "Failed to initialize button support\n");
        return 1;
    }
    
    // Set button callback
    inky_button_set_callback(button_callback, NULL);
    
    printf("\nTesting emulated button presses...\n");
    
    int cycle = 0;
    while (running) {
        // Simulate button presses in sequence every 2 seconds
        int button = cycle % 4;
        printf("\nCycle %d: Simulating button %c press...\n", cycle + 1, 'A' + button);
        inky_button_emulate_press(button);
        
        // Show button state
        printf("Button states after emulation: A:%s B:%s C:%s D:%s\n",
               inky_button_is_pressed(INKY_BUTTON_A) ? "●" : "○",
               inky_button_is_pressed(INKY_BUTTON_B) ? "●" : "○",
               inky_button_is_pressed(INKY_BUTTON_C) ? "●" : "○",
               inky_button_is_pressed(INKY_BUTTON_D) ? "●" : "○");
        
        // Wait 2 seconds before next test
        for (int i = 0; i < 20 && running; i++) {
            usleep(100000);  // 100ms
        }
        
        cycle++;
        
        // Reset after testing all 4 buttons
        if (cycle >= 8) {  // Test each button twice
            printf("\nAll emulated button tests completed. Press Ctrl+C to exit.\n");
            break;
        }
    }
    
    printf("\nCleaning up...\n");
    inky_button_cleanup();
    
    printf("Emulated button test completed.\n");
    return 0;
}