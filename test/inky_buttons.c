#include "inky_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#ifdef __linux__
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#else
#include <sys/ioctl.h>  // For ioctl stub definitions
// Stub definitions for non-Linux platforms
#define GPIOHANDLE_REQUEST_INPUT 0
#define GPIOHANDLE_REQUEST_BIAS_PULL_UP 0
#define GPIO_GET_LINEHANDLE_IOCTL 0
#define GPIOHANDLE_GET_LINE_VALUES_IOCTL 0

struct gpiohandle_request {
    uint32_t lineoffsets[64];
    uint32_t flags;
    uint8_t default_values[64];
    char consumer_label[32];
    uint32_t lines;
    int fd;
};

struct gpiohandle_data {
    uint8_t values[64];
};
#endif

#define GPIO_DEVICE "/dev/gpiochip0"
#define DEBOUNCE_MS 50  // 50ms debounce time

// Button state structure
typedef struct {
    int gpio_pin;
    int gpio_fd;
    bool last_state;
    uint64_t last_change_time;
    bool is_pressed;
} button_state_t;

// Global button state
static struct {
    bool initialized;
    bool emulator_mode;
    int gpio_chip_fd;
    button_state_t buttons[4];
    inky_button_callback_t callback;
    void *user_data;
} button_ctx = {0};

// Get current time in milliseconds
static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Read GPIO value - returns true when button is pressed
static bool read_button_gpio(int fd) {
#ifdef __linux__
    struct gpiohandle_data data;
    if (ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0) {
        return false;  // Default to not pressed on error
    }
    return data.values[0] == 0;  // Button pressed = GPIO low = true
#else
    (void)fd;
    return false;  // Not supported on non-Linux
#endif
}

int inky_button_init(void) {
    if (button_ctx.initialized) {
        return 0;  // Already initialized
    }
    
#ifndef __linux__
    // Non-Linux platforms use emulator mode
    button_ctx.emulator_mode = true;
    printf("Button support initialized (emulator mode)\n");
    goto init_common;
#endif
    
    // Button GPIO pins
    int button_pins[4] = {
        INKY_BUTTON_A_PIN,
        INKY_BUTTON_B_PIN,
        INKY_BUTTON_C_PIN,
        INKY_BUTTON_D_PIN
    };
    
    // Open GPIO chip
    button_ctx.gpio_chip_fd = open(GPIO_DEVICE, O_RDONLY);
    if (button_ctx.gpio_chip_fd < 0) {
        perror("Failed to open GPIO chip for buttons");
        return -1;
    }
    
    button_ctx.emulator_mode = false;  // Hardware mode
    
    // Initialize each button
    for (int i = 0; i < 4; i++) {
        button_ctx.buttons[i].gpio_pin = button_pins[i];
        
        // Request GPIO line for input with pull-up
        struct gpiohandle_request req;
        req.lineoffsets[0] = button_pins[i];
        req.lines = 1;
        req.flags = GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_BIAS_PULL_UP;
        snprintf(req.consumer_label, sizeof(req.consumer_label), "inky_btn_%c", 'A' + i);
        
        if (ioctl(button_ctx.gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
            fprintf(stderr, "Failed to request GPIO line for button %c\n", 'A' + i);
            
            // Clean up already initialized buttons
            for (int j = 0; j < i; j++) {
                close(button_ctx.buttons[j].gpio_fd);
            }
            close(button_ctx.gpio_chip_fd);
            return -1;
        }
        
        button_ctx.buttons[i].gpio_fd = req.fd;
        
        // Initialize state by reading actual GPIO value
        button_ctx.buttons[i].last_state = read_button_gpio(button_ctx.buttons[i].gpio_fd);
        button_ctx.buttons[i].last_change_time = get_time_ms();
        button_ctx.buttons[i].is_pressed = false;
        
        printf("Button %c initialized: GPIO=%d, initial_state=%d\n", 
               'A' + i, button_pins[i], button_ctx.buttons[i].last_state);
    }
    
    printf("Button support initialized (A=GPIO%d, B=GPIO%d, C=GPIO%d, D=GPIO%d)\n",
           INKY_BUTTON_A_PIN, INKY_BUTTON_B_PIN, INKY_BUTTON_C_PIN, INKY_BUTTON_D_PIN);
    
    goto init_common;

init_common:
    // Common initialization for both hardware and emulator
    if (button_ctx.emulator_mode) {
        // Initialize emulator button states
        for (int i = 0; i < 4; i++) {
            button_ctx.buttons[i].gpio_pin = -1;  // No GPIO
            button_ctx.buttons[i].gpio_fd = -1;   // No file descriptor
            button_ctx.buttons[i].last_state = false;  // Not pressed
            button_ctx.buttons[i].last_change_time = get_time_ms();
            button_ctx.buttons[i].is_pressed = false;
        }
    }
    
    button_ctx.initialized = true;
    button_ctx.callback = NULL;
    button_ctx.user_data = NULL;
    
    return 0;
}

void inky_button_set_callback(inky_button_callback_t callback, void *user_data) {
    if (!button_ctx.initialized) return;
    
    button_ctx.callback = callback;
    button_ctx.user_data = user_data;
}

void inky_button_poll(void) {
    if (!button_ctx.initialized) return;
    
    // Emulator mode doesn't need to poll - emulated presses are handled directly
    if (button_ctx.emulator_mode) {
        return;
    }
    
    uint64_t current_time = get_time_ms();
    
    for (int i = 0; i < 4; i++) {
        button_state_t *btn = &button_ctx.buttons[i];
        
        // Read current GPIO state
        bool current_state = read_button_gpio(btn->gpio_fd);
        
        // Removed verbose GPIO state debug output
        
        // Check if state changed
        if (current_state != btn->last_state) {
            btn->last_change_time = current_time;
            btn->last_state = current_state;
        }
        
        // Apply debouncing
        if ((current_time - btn->last_change_time) >= DEBOUNCE_MS) {
            bool new_pressed = current_state;  // With pull-up bias, pressed = GPIO low = false, so use direct state
            
            // Detect button press (transition from not pressed to pressed)
            if (new_pressed && !btn->is_pressed) {
                btn->is_pressed = true;
                printf("DEBUG: Button %c PRESS detected (GPIO:%d, pressed:%d)\n", 'A' + i, current_state, new_pressed);
                
                // Call callback if registered
                if (button_ctx.callback) {
                    button_ctx.callback(i, button_ctx.user_data);
                }
            }
            // Detect button release
            else if (!new_pressed && btn->is_pressed) {
                btn->is_pressed = false;
                printf("DEBUG: Button %c RELEASE detected (GPIO:%d, pressed:%d)\n", 'A' + i, current_state, new_pressed);
            }
        }
    }
}

bool inky_button_is_pressed(int button) {
    if (!button_ctx.initialized || button < 0 || button >= 4) {
        return false;
    }
    
    return button_ctx.buttons[button].is_pressed;
}

void inky_button_cleanup(void) {
    if (!button_ctx.initialized) return;
    
    if (!button_ctx.emulator_mode) {
        // Close all button GPIO file descriptors (hardware mode only)
        for (int i = 0; i < 4; i++) {
            if (button_ctx.buttons[i].gpio_fd > 0) {
                close(button_ctx.buttons[i].gpio_fd);
                button_ctx.buttons[i].gpio_fd = 0;
            }
        }
        
        // Close GPIO chip
        if (button_ctx.gpio_chip_fd > 0) {
            close(button_ctx.gpio_chip_fd);
            button_ctx.gpio_chip_fd = 0;
        }
    }
    
    button_ctx.initialized = false;
    button_ctx.emulator_mode = false;
    button_ctx.callback = NULL;
    button_ctx.user_data = NULL;
    
    printf("Button support cleaned up\n");
}

void inky_button_emulate_press(int button) {
    if (!button_ctx.initialized) {
        printf("WARNING: Buttons not initialized - call inky_button_init() first\n");
        return;
    }
    
    if (button < 0 || button >= 4) {
        printf("WARNING: Invalid button %d (must be 0-3)\n", button);
        return;
    }
    
    if (!button_ctx.emulator_mode) {
        printf("WARNING: inky_button_emulate_press() only works in emulator mode\n");
        return;
    }
    
    const char *button_names[] = {"A", "B", "C", "D"};
    printf("Emulating button %s press\n", button_names[button]);
    
    // Trigger the callback immediately
    if (button_ctx.callback) {
        button_ctx.callback(button, button_ctx.user_data);
    }
    
    // Update the button state for is_pressed() queries
    button_ctx.buttons[button].is_pressed = true;
    button_ctx.buttons[button].last_change_time = get_time_ms();
    
    // Note: In emulator mode, we don't automatically release the button
    // The application can call inky_button_emulate_press again or 
    // check inky_button_is_pressed() to manage state
}