#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s [--emulator|--hardware]\n", prog_name);
    printf("This program demonstrates proper ghosting management with partial updates.\n");
}

int main(int argc, char *argv[]) {
    // Default behavior based on build type
#ifdef HARDWARE_BUILD
    bool use_emulator = false;
#else
    bool use_emulator = true;
#endif
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--emulator") == 0) {
            use_emulator = true;
        } else if (strcmp(argv[i], "--hardware") == 0) {
            use_emulator = false;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("Inky Ghosting Management Demo\n");
    printf("=============================\n");
    printf("Mode: %s\n", use_emulator ? "Emulator" : "Hardware");
    printf("This demo shows proper partial update usage with ghosting prevention.\n\n");
    
    // Initialize display
    printf("Initializing display...\n");
    inky_t *display = inky_init(use_emulator);
    if (!display) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }
    
    // Clear display initially
    printf("Initial setup with full refresh...\n");
    inky_clear(display, INKY_WHITE);
    
    // Draw static elements that won't change
    uint16_t width = inky_get_width(display);
    uint16_t height = inky_get_height(display);
    
    // Border
    for (int x = 0; x < width; x++) {
        inky_set_pixel(display, x, 0, INKY_BLACK);
        inky_set_pixel(display, x, height - 1, INKY_BLACK);
    }
    for (int y = 0; y < height; y++) {
        inky_set_pixel(display, 0, y, INKY_BLACK);
        inky_set_pixel(display, width - 1, y, INKY_BLACK);
    }
    
    // Title
    for (int i = 0; i < 20; i++) {
        for (int y = 0; y < 15; y++) {
            for (int x = 0; x < 8; x++) {
                inky_set_pixel(display, 50 + i * 12 + x, 30 + y, INKY_RED);
            }
        }
    }
    
    // Full update to establish baseline
    inky_update(display);
    if (use_emulator) {
        inky_emulator_save_ppm(display, "ghosting_demo_initial.ppm");
    }
    
    printf("\nStarting counter demo with smart ghosting management...\n");
    
    // Counter area dimensions
    int counter_x = 250, counter_y = 200, counter_w = 100, counter_h = 50;
    
    for (int count = 1; count <= 20; count++) {
        printf("\n--- Update %d ---\n", count);
        
        // Check if we should do a full refresh
        if (inky_should_full_refresh(display)) {
            printf("🔄 SMART DECISION: Full refresh recommended (partial count: %d)\n", 
                   inky_get_partial_count(display));
            
            // Do full refresh to clear any ghosting
            inky_update(display);
            
            if (use_emulator) {
                char filename[64];
                snprintf(filename, sizeof(filename), "ghosting_demo_full_refresh_%d.ppm", count);
                inky_emulator_save_ppm(display, filename);
            }
            
            if (!use_emulator) {
                printf("⏳ Waiting for full refresh to complete...\n");
            }
        }
        
        // Clear counter area
        for (int y = counter_y; y < counter_y + counter_h; y++) {
            for (int x = counter_x; x < counter_x + counter_w; x++) {
                inky_set_pixel(display, x, y, INKY_WHITE);
            }
        }
        
        // Draw counter with different colors
        uint8_t color = (count % 6) + 2;  // Colors 2-7 (skip black/white)
        for (int y = 0; y < 30; y++) {
            for (int x = 0; x < 60; x++) {
                if ((x + y + count) % 5 == 0) {  // Pattern based on count
                    inky_set_pixel(display, counter_x + 20 + x, counter_y + 10 + y, color);
                }
            }
        }
        
        // Add count indicator (simple bars)
        for (int i = 0; i < (count % 10); i++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 4; x++) {
                    inky_set_pixel(display, counter_x + 5 + i * 6 + x, counter_y + 35 + y, INKY_BLACK);
                }
            }
        }
        
        printf("Counter: %d (color: %d)\n", count, color);
        printf("Partial update count before this update: %d\n", inky_get_partial_count(display));
        
        // Partial update of counter area
        inky_update_region(display, counter_x, counter_y, counter_w, counter_h);
        
        if (use_emulator) {
            char filename[64];
            snprintf(filename, sizeof(filename), "ghosting_demo_counter_%02d.ppm", count);
            inky_emulator_save_ppm(display, filename);
        }
        
        printf("Partial update count after this update: %d\n", inky_get_partial_count(display));
        
        // Show recommendation
        if (inky_should_full_refresh(display)) {
            printf("💡 RECOMMENDATION: Consider full refresh before next update\n");
        } else {
            printf("✅ SAFE: Can continue with partial updates\n");
        }
        
        // Small delay for hardware mode
        if (!use_emulator) {
            printf("⏳ Waiting for partial update to complete...\n");
            sleep(1);
        }
    }
    
    printf("\n🎯 FINAL DEMONSTRATION: Automatic ghosting cleanup\n");
    printf("Even though we could continue with partial updates,\n");
    printf("let's do a final full refresh to show the difference:\n");
    
    inky_update(display);
    if (use_emulator) {
        inky_emulator_save_ppm(display, "ghosting_demo_final_clean.ppm");
    }
    
    printf("\n📊 SUMMARY:\n");
    printf("- Performed 20 partial updates with smart ghosting management\n");
    printf("- Full refreshes were automatically triggered when needed\n");
    printf("- Final partial count: %d\n", inky_get_partial_count(display));
    printf("- This prevents ghosting while maximizing performance!\n");
    
    // Clean up
    printf("\nCleaning up...\n");
    inky_destroy(display);
    
    printf("Ghosting management demo completed!\n");
    return 0;
}