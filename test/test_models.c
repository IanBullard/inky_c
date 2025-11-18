#include "inky.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing Inky Display Model Support\n");
    printf("===================================\n\n");

    // Test 5.7" display initialization
    printf("1. Testing 5.7\" display (600x448):\n");
    inky_t *display_5_7 = inky_init_model(INKY_IMPRESSION_5_7, true);
    if (display_5_7) {
        printf("   - Initialized successfully\n");
        printf("   - Width: %d, Height: %d\n", inky_get_width(display_5_7), inky_get_height(display_5_7));

        // Draw a test pattern
        inky_clear(display_5_7, INKY_WHITE);
        for (int x = 100; x < 500; x += 50) {
            for (int y = 100; y < 348; y += 50) {
                inky_set_pixel(display_5_7, x, y, INKY_BLACK);
            }
        }
        inky_emulator_save_ppm(display_5_7, "test_5_7.ppm");
        printf("   - Saved test image to test_5_7.ppm\n");

        inky_destroy(display_5_7);
        printf("   - Cleaned up successfully\n\n");
    } else {
        printf("   - Failed to initialize\n\n");
    }

    // Test 13.3" display initialization
    printf("2. Testing 13.3\" display (1600x1200):\n");
    inky_t *display_13_3 = inky_init_model(INKY_IMPRESSION_13_3, true);
    if (display_13_3) {
        printf("   - Initialized successfully\n");
        printf("   - Width: %d, Height: %d\n", inky_get_width(display_13_3), inky_get_height(display_13_3));

        // Draw a test pattern
        inky_clear(display_13_3, INKY_WHITE);
        for (int x = 200; x < 1400; x += 100) {
            for (int y = 200; y < 1000; y += 100) {
                // Draw a small cross
                for (int i = -10; i <= 10; i++) {
                    inky_set_pixel(display_13_3, x + i, y, INKY_RED);
                    inky_set_pixel(display_13_3, x, y + i, INKY_RED);
                }
            }
        }
        inky_emulator_save_ppm(display_13_3, "test_13_3.ppm");
        printf("   - Saved test image to test_13_3.ppm\n");

        inky_destroy(display_13_3);
        printf("   - Cleaned up successfully\n\n");
    } else {
        printf("   - Failed to initialize\n\n");
    }

    // Test auto-detection (defaults to 5.7" in emulator)
    printf("3. Testing auto-detection (emulator defaults to 5.7\"):\n");
    inky_t *display_auto = inky_init(true);
    if (display_auto) {
        printf("   - Initialized successfully\n");
        printf("   - Width: %d, Height: %d\n", inky_get_width(display_auto), inky_get_height(display_auto));
        inky_destroy(display_auto);
        printf("   - Cleaned up successfully\n\n");
    } else {
        printf("   - Failed to initialize\n\n");
    }

    printf("All tests completed!\n");
    return 0;
}