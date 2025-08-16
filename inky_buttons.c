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

// Read GPIO value
static bool read_button_gpio(int fd) {
#ifdef __linux__
    struct gpiohandle_data data;
    if (ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0) {
        return true;  // Default to not pressed on error
    }
    return data.values[0] == 0;  // Button pressed = low (pull-up)
#else
    (void)fd;
    return false;  // Not supported on non-Linux
#endif
}

int inky_button_init(void) {
#ifndef __linux__
    // Buttons not supported on non-Linux platforms
    return -1;
#endif
    
    if (button_ctx.initialized) {
        return 0;  // Already initialized
    }
    
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
    
    // Initialize each button
    for (int i = 0; i < 4; i++) {
        button_ctx.buttons[i].gpio_pin = button_pins[i];
        
#ifdef __linux__
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
#endif
        
        // Initialize state by reading actual GPIO value
        button_ctx.buttons[i].last_state = read_button_gpio(button_ctx.buttons[i].gpio_fd);
        button_ctx.buttons[i].last_change_time = get_time_ms();
        button_ctx.buttons[i].is_pressed = false;
        
        printf("Button %c initialized: GPIO=%d, initial_state=%d\n", 
               'A' + i, button_pins[i], button_ctx.buttons[i].last_state);
    }
    
    button_ctx.initialized = true;
    button_ctx.callback = NULL;
    button_ctx.user_data = NULL;
    
    printf("Button support initialized (A=GPIO%d, B=GPIO%d, C=GPIO%d, D=GPIO%d)\n",
           INKY_BUTTON_A_PIN, INKY_BUTTON_B_PIN, INKY_BUTTON_C_PIN, INKY_BUTTON_D_PIN);
    
    return 0;
}

void inky_button_set_callback(inky_button_callback_t callback, void *user_data) {
    if (!button_ctx.initialized) return;
    
    button_ctx.callback = callback;
    button_ctx.user_data = user_data;
}

void inky_button_poll(void) {
    if (!button_ctx.initialized) return;
    
    uint64_t current_time = get_time_ms();
    static uint64_t last_debug_time = 0;
    bool debug_print = (current_time - last_debug_time) > 5000;  // Debug every 5 seconds
    
    for (int i = 0; i < 4; i++) {
        button_state_t *btn = &button_ctx.buttons[i];
        
        // Read current GPIO state
        bool current_state = read_button_gpio(btn->gpio_fd);
        
        if (debug_print && i == 0) {
            printf("DEBUG: Button states (raw GPIO): A:%d B:%d C:%d D:%d\n", 
                   read_button_gpio(button_ctx.buttons[0].gpio_fd),
                   read_button_gpio(button_ctx.buttons[1].gpio_fd),
                   read_button_gpio(button_ctx.buttons[2].gpio_fd),
                   read_button_gpio(button_ctx.buttons[3].gpio_fd));
            last_debug_time = current_time;
        }
        
        // Check if state changed
        if (current_state != btn->last_state) {
            printf("DEBUG: Button %c state change: %d -> %d\n", 'A' + i, btn->last_state, current_state);
            btn->last_change_time = current_time;
            btn->last_state = current_state;
        }
        
        // Apply debouncing
        if ((current_time - btn->last_change_time) >= DEBOUNCE_MS) {
            bool new_pressed = !current_state;  // Inverted because of pull-up
            
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
    
    // Close all button GPIO file descriptors
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
    
    button_ctx.initialized = false;
    button_ctx.callback = NULL;
    button_ctx.user_data = NULL;
    
    printf("Button support cleaned up\n");
}