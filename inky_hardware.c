/* 
 * Hardware implementation for Inky display on Raspberry Pi
 * This file should be compiled on Linux systems with proper SPI/GPIO support
 */

#define _GNU_SOURCE
#include "inky_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>

#ifdef __linux__
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
#else
// Stub definitions for non-Linux platforms
#define GPIOHANDLE_REQUEST_OUTPUT 0
#define GPIOHANDLE_REQUEST_INPUT 0
#define GPIO_GET_LINEHANDLE_IOCTL 0
#define GPIOHANDLE_SET_LINE_VALUES_IOCTL 0
#define GPIOHANDLE_GET_LINE_VALUES_IOCTL 0
#define SPI_IOC_WR_MODE 0
#define SPI_IOC_WR_BITS_PER_WORD 0
#define SPI_IOC_WR_MAX_SPEED_HZ 0
#define SPI_NO_CS 0

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

#define SPI_DEVICE "/dev/spidev0.0"
#define GPIO_DEVICE "/dev/gpiochip0"
#define SPI_SPEED_HZ 3000000
#define SPI_MODE 0
#define SPI_BITS_PER_WORD 8

inky_t* inky_init(bool emulator) {
    if (emulator) {
        fprintf(stderr, "Error: This is the hardware build. Use the emulator build for emulation.\n");
        return NULL;
    }
    
    // Use common initialization
    inky_t *display = inky_init_common(false);
    if (!display) {
        return NULL;
    }
    
    // Initialize SPI
    display->spi_fd = open(SPI_DEVICE, O_RDWR);
    if (display->spi_fd < 0) {
        perror("Failed to open SPI device");
        inky_destroy_common(display);
        return NULL;
    }
    
    // Configure SPI
    uint8_t mode = SPI_MODE | SPI_NO_CS;  // Disable hardware CS, we'll control it via GPIO
    uint8_t bits = SPI_BITS_PER_WORD;
    uint32_t speed = SPI_SPEED_HZ;
    
    if (ioctl(display->spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(display->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(display->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to configure SPI");
        close(display->spi_fd);
        inky_destroy_common(display);
        return NULL;
    }
    
    // Initialize GPIO
    if (!inky_hw_init_gpio(display)) {
        close(display->spi_fd);
        inky_destroy_common(display);
        return NULL;
    }
    
    // Setup the display
    inky_hw_setup(display);
    
    return display;
}

void inky_destroy(inky_t *display) {
    if (!display) return;
    
    if (!display->is_emulator) {
        if (display->reset_line > 0) close(display->reset_line);
        if (display->dc_line > 0) close(display->dc_line);
        if (display->cs_line > 0) close(display->cs_line);
        if (display->busy_line > 0) close(display->busy_line);
        if (display->gpio_chip_fd > 0) close(display->gpio_chip_fd);
        if (display->spi_fd > 0) close(display->spi_fd);
    }
    
    inky_destroy_common(display);
}

bool inky_hw_init_gpio(inky_t *display) {
    display->gpio_chip_fd = open(GPIO_DEVICE, O_RDONLY);
    if (display->gpio_chip_fd < 0) {
        perror("Failed to open GPIO chip");
        return false;
    }
    
    // Request RESET GPIO line
    struct gpiohandle_request reset_req;
    reset_req.lineoffsets[0] = INKY_RESET_PIN;
    reset_req.lines = 1;
    reset_req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    reset_req.default_values[0] = 1;  // Reset high
    strcpy(reset_req.consumer_label, "inky_reset");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &reset_req) < 0) {
        perror("Failed to request RESET GPIO line");
        return false;
    }
    display->reset_line = reset_req.fd;
    
    // Request DC GPIO line
    struct gpiohandle_request dc_req;
    dc_req.lineoffsets[0] = INKY_DC_PIN;
    dc_req.lines = 1;
    dc_req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    dc_req.default_values[0] = 0;  // DC low
    strcpy(dc_req.consumer_label, "inky_dc");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &dc_req) < 0) {
        perror("Failed to request DC GPIO line");
        close(display->reset_line);
        return false;
    }
    display->dc_line = dc_req.fd;
    
    // Request CS GPIO line
    struct gpiohandle_request cs_req;
    cs_req.lineoffsets[0] = INKY_CS_PIN;
    cs_req.lines = 1;
    cs_req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    cs_req.default_values[0] = 1;  // CS high (inactive)
    strcpy(cs_req.consumer_label, "inky_cs");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &cs_req) < 0) {
        perror("Failed to request CS GPIO line");
        close(display->dc_line);
        close(display->reset_line);
        return false;
    }
    display->cs_line = cs_req.fd;
    
    // Request BUSY pin as input
    struct gpiohandle_request busy_req;
    busy_req.lineoffsets[0] = INKY_BUSY_PIN;
    busy_req.lines = 1;
    busy_req.flags = GPIOHANDLE_REQUEST_INPUT;
    strcpy(busy_req.consumer_label, "inky_busy");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &busy_req) < 0) {
        perror("Failed to request BUSY GPIO line");
        close(display->cs_line);
        close(display->dc_line);
        close(display->reset_line);
        return false;
    }
    display->busy_line = busy_req.fd;
    
    return true;
}

static void gpio_set_value(int fd, int value) {
    struct gpiohandle_data data;
    data.values[0] = value;
    ioctl(fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
}

static int gpio_get_value(int fd) {
    struct gpiohandle_data data;
    ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    return data.values[0];
}

void inky_hw_send_command(inky_t *display, uint8_t command) {
    if (!display || display->is_emulator) return;
    
    // Set DC low for command
    gpio_set_value(display->dc_line, 0);
    
    // Set CS low (active)
    gpio_set_value(display->cs_line, 0);
    
    // Send command byte
    write(display->spi_fd, &command, 1);
    
    // Set CS high (inactive)
    gpio_set_value(display->cs_line, 1);
}

void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len) {
    if (!display || display->is_emulator || !data || len == 0) return;
    
    // Set DC high for data
    gpio_set_value(display->dc_line, 1);
    
    // Set CS low (active)
    gpio_set_value(display->cs_line, 0);
    
    // Send data in chunks if needed
    const size_t chunk_size = 4096;
    size_t offset = 0;
    while (offset < len) {
        size_t to_send = (len - offset) > chunk_size ? chunk_size : (len - offset);
        write(display->spi_fd, data + offset, to_send);
        offset += to_send;
    }
    
    // Set CS high (inactive)
    gpio_set_value(display->cs_line, 1);
}

void inky_hw_busy_wait(inky_t *display) {
    if (!display || display->is_emulator) return;
    
    // Wait for busy pin to go high
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    while (1) {
        if (gpio_get_value(display->busy_line) == 1) {
            break;  // Display is ready
        }
        
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed > 40.0) {
            fprintf(stderr, "Warning: Busy wait timeout after 40 seconds\n");
            break;
        }
        
        usleep(10000);  // Sleep for 10ms
    }
}

void inky_hw_reset(inky_t *display) {
    if (!display || display->is_emulator) return;
    
    // Reset sequence
    gpio_set_value(display->reset_line, 0);  // Reset low
    usleep(100000);  // 100ms
    gpio_set_value(display->reset_line, 1);  // Reset high
    usleep(100000);  // 100ms
    
    inky_hw_busy_wait(display);
}

void inky_hw_setup(inky_t *display) {
    if (!display || display->is_emulator) return;
    
    // Reset the display
    inky_hw_reset(display);
    
    // Send initialization commands
    
    // Resolution Setting (600x448)
    inky_hw_send_command(display, UC8159_TRES);
    uint8_t res_data[4] = {
        (display->width >> 8) & 0xFF,
        display->width & 0xFF,
        (display->height >> 8) & 0xFF,
        display->height & 0xFF
    };
    inky_hw_send_data(display, res_data, 4);
    
    // Panel Setting
    // 0b11 = 600x448 resolution
    inky_hw_send_command(display, UC8159_PSR);
    uint8_t psr_data[2] = {
        (0x03 << 6) | 0x2F,  // Resolution 600x448, other settings
        0x08                  // 7-color mode
    };
    inky_hw_send_data(display, psr_data, 2);
    
    // Power Settings
    inky_hw_send_command(display, UC8159_PWR);
    uint8_t pwr_data[4] = {0x07, 0x00, 0x23, 0x23};
    inky_hw_send_data(display, pwr_data, 4);
    
    // PLL Control
    inky_hw_send_command(display, UC8159_PLL);
    uint8_t pll_data = 0x3C;
    inky_hw_send_data(display, &pll_data, 1);
    
    // TSE
    inky_hw_send_command(display, UC8159_TSE);
    uint8_t tse_data = 0x00;
    inky_hw_send_data(display, &tse_data, 1);
    
    // VCOM and Data Interval
    inky_hw_send_command(display, UC8159_CDI);
    uint8_t cdi_data = (display->border_color << 5) | 0x17;
    inky_hw_send_data(display, &cdi_data, 1);
    
    // TCON
    inky_hw_send_command(display, UC8159_TCON);
    uint8_t tcon_data = 0x22;
    inky_hw_send_data(display, &tcon_data, 1);
    
    // DAM - Disable external flash
    inky_hw_send_command(display, UC8159_DAM);
    uint8_t dam_data = 0x00;
    inky_hw_send_data(display, &dam_data, 1);
    
    // PWS
    inky_hw_send_command(display, UC8159_PWS);
    uint8_t pws_data = 0xAA;
    inky_hw_send_data(display, &pws_data, 1);
    
    // Power off sequence
    inky_hw_send_command(display, UC8159_PFS);
    uint8_t pfs_data = 0x00;
    inky_hw_send_data(display, &pfs_data, 1);
}

void inky_hw_update(inky_t *display) {
    if (!display || display->is_emulator) return;
    
    // Send display data
    inky_hw_send_command(display, UC8159_DTM1);
    inky_hw_send_data(display, display->buffer, display->buffer_size);
    
    // Power on
    inky_hw_send_command(display, UC8159_PON);
    usleep(200000);  // 200ms
    
    // Display refresh
    inky_hw_send_command(display, UC8159_DRF);
    inky_hw_busy_wait(display);  // This can take up to 32 seconds
    
    // Power off
    inky_hw_send_command(display, UC8159_POF);
    usleep(200000);  // 200ms
}

void inky_hw_set_partial_window(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!display || display->is_emulator) return;
    
    // Validate bounds
    if (x >= display->width || y >= display->height || 
        x + width > display->width || y + height > display->height) {
        printf("WARNING: Partial window coordinates out of bounds\n");
        return;
    }
    
    // Set partial window command (0x90)
    // Parameters: x_start (2 bytes), y_start (2 bytes), x_end (2 bytes), y_end (2 bytes)
    inky_hw_send_command(display, UC8159_PARTIAL_WINDOW);
    uint8_t window_data[8] = {
        (x >> 8) & 0xFF,              // x_start high byte
        x & 0xFF,                     // x_start low byte
        (y >> 8) & 0xFF,              // y_start high byte  
        y & 0xFF,                     // y_start low byte
        ((x + width - 1) >> 8) & 0xFF,  // x_end high byte
        (x + width - 1) & 0xFF,       // x_end low byte
        ((y + height - 1) >> 8) & 0xFF, // y_end high byte
        (y + height - 1) & 0xFF       // y_end low byte
    };
    inky_hw_send_data(display, window_data, 8);
    
    printf("Set partial window: (%d,%d) to (%d,%d)\n", x, y, x + width - 1, y + height - 1);
}

void inky_hw_partial_update(inky_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (!display || display->is_emulator) return;
    
    // Validate bounds
    if (x >= display->width || y >= display->height || 
        x + width > display->width || y + height > display->height) {
        printf("ERROR: Partial update coordinates out of bounds\n");
        return;
    }
    
    printf("Starting partial update for region (%d,%d) %dx%d\n", x, y, width, height);
    
    // Set up the display for partial update
    inky_hw_setup(display);
    
    // Set partial window
    inky_hw_set_partial_window(display, x, y, width, height);
    
    // Enter partial update mode
    inky_hw_send_command(display, UC8159_PARTIAL_IN);
    
    // Extract the region data from the full buffer
    size_t region_size = (width * height + 1) / 2;  // 4-bit packed pixels
    uint8_t *region_buffer = malloc(region_size);
    if (!region_buffer) {
        printf("ERROR: Failed to allocate region buffer\n");
        return;
    }
    
    // Copy pixels from the region to the temporary buffer
    size_t pixel_idx = 0;
    for (uint16_t row = y; row < y + height; row++) {
        for (uint16_t col = x; col < x + width; col++) {
            // Get pixel from main buffer
            size_t main_pixel_idx = row * display->width + col;
            size_t main_byte_idx = main_pixel_idx / 2;
            uint8_t pixel_value;
            
            if (main_pixel_idx & 1) {
                // Odd pixel - low nibble
                pixel_value = display->buffer[main_byte_idx] & 0x0F;
            } else {
                // Even pixel - high nibble
                pixel_value = (display->buffer[main_byte_idx] >> 4) & 0x0F;
            }
            
            // Store in region buffer
            size_t region_byte_idx = pixel_idx / 2;
            if (pixel_idx & 1) {
                // Odd pixel - low nibble
                region_buffer[region_byte_idx] = (region_buffer[region_byte_idx] & 0xF0) | (pixel_value & 0x0F);
            } else {
                // Even pixel - high nibble
                region_buffer[region_byte_idx] = (region_buffer[region_byte_idx] & 0x0F) | ((pixel_value & 0x0F) << 4);
            }
            
            pixel_idx++;
        }
    }
    
    // Send region data
    inky_hw_send_command(display, UC8159_DTM1);
    inky_hw_send_data(display, region_buffer, region_size);
    
    // Power on
    inky_hw_send_command(display, UC8159_PON);
    usleep(200000);  // 200ms
    
    // Display refresh (should be faster for partial updates)
    inky_hw_send_command(display, UC8159_DRF);
    inky_hw_busy_wait(display);  // Partial updates are typically 2-4 seconds
    
    // Power off
    inky_hw_send_command(display, UC8159_POF);
    usleep(200000);  // 200ms
    
    // Exit partial update mode
    inky_hw_send_command(display, UC8159_PARTIAL_OUT);
    
    free(region_buffer);
    printf("Partial update completed\n");
}

