/* 
 * Hardware implementation for Inky display on Linux (Raspberry Pi)
 * This file should be compiled on Linux systems with proper SPI/GPIO support
 */

#define _GNU_SOURCE
#include "inky.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>

#define SPI_DEVICE "/dev/spidev0.0"
#define GPIO_DEVICE "/dev/gpiochip0"
#define SPI_SPEED_HZ 3000000
#define SPI_MODE 0
#define SPI_BITS_PER_WORD 8

inky_t* inky_init(bool emulator) {
    if (emulator) {
        // Emulator initialization is in inky_emulator.c
        return NULL;
    }
    
    inky_t *display = calloc(1, sizeof(inky_t));
    if (!display) {
        return NULL;
    }
    
    display->width = INKY_WIDTH;
    display->height = INKY_HEIGHT;
    display->is_emulator = false;
    display->border_color = INKY_WHITE;
    display->h_flip = false;
    display->v_flip = false;
    
    // Calculate buffer size - 4 bits per pixel, packed
    display->buffer_size = (display->width * display->height + 1) / 2;
    display->buffer = calloc(display->buffer_size, 1);
    
    if (!display->buffer) {
        free(display);
        return NULL;
    }
    
    // Initialize to white
    memset(display->buffer, 0x11, display->buffer_size);
    
    // Initialize SPI
    display->spi_fd = open(SPI_DEVICE, O_RDWR);
    if (display->spi_fd < 0) {
        perror("Failed to open SPI device");
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    // Configure SPI
    uint8_t mode = SPI_MODE;
    uint8_t bits = SPI_BITS_PER_WORD;
    uint32_t speed = SPI_SPEED_HZ;
    
    if (ioctl(display->spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(display->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(display->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to configure SPI");
        close(display->spi_fd);
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    // Initialize GPIO
    display->gpio_chip_fd = open(GPIO_DEVICE, O_RDONLY);
    if (display->gpio_chip_fd < 0) {
        perror("Failed to open GPIO chip");
        close(display->spi_fd);
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    // Request GPIO lines
    struct gpiohandle_request req;
    req.lineoffsets[0] = INKY_RESET_PIN;
    req.lineoffsets[1] = INKY_DC_PIN;
    req.lineoffsets[2] = INKY_CS_PIN;
    req.lines = 3;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.default_values[0] = 1;  // Reset high
    req.default_values[1] = 0;  // DC low
    req.default_values[2] = 1;  // CS high (inactive)
    strcpy(req.consumer_label, "inky");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        perror("Failed to request GPIO lines for output");
        close(display->gpio_chip_fd);
        close(display->spi_fd);
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    display->reset_line = req.fd;
    
    // Request BUSY pin as input
    struct gpiohandle_request busy_req;
    busy_req.lineoffsets[0] = INKY_BUSY_PIN;
    busy_req.lines = 1;
    busy_req.flags = GPIOHANDLE_REQUEST_INPUT;
    strcpy(busy_req.consumer_label, "inky_busy");
    
    if (ioctl(display->gpio_chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &busy_req) < 0) {
        perror("Failed to request BUSY GPIO line");
        close(display->reset_line);
        close(display->gpio_chip_fd);
        close(display->spi_fd);
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    display->busy_line = busy_req.fd;
    
    // Setup the display
    inky_hw_setup(display);
    
    return display;
}

void inky_destroy(inky_t *display) {
    if (!display) return;
    
    if (display->buffer) {
        free(display->buffer);
    }
    
    if (!display->is_emulator) {
        if (display->reset_line > 0) close(display->reset_line);
        if (display->busy_line > 0) close(display->busy_line);
        if (display->gpio_chip_fd > 0) close(display->gpio_chip_fd);
        if (display->spi_fd > 0) close(display->spi_fd);
    }
    
    free(display);
}

void inky_clear(inky_t *display, uint8_t color) {
    if (!display || !display->buffer) return;
    
    // Pack two pixels of the same color into one byte
    uint8_t packed_color = ((color & 0x0F) << 4) | (color & 0x0F);
    memset(display->buffer, packed_color, display->buffer_size);
}

void inky_set_pixel(inky_t *display, uint16_t x, uint16_t y, uint8_t color) {
    if (!display || !display->buffer) return;
    if (x >= display->width || y >= display->height) return;
    
    size_t pixel_index = y * display->width + x;
    size_t byte_index = pixel_index / 2;
    
    if (pixel_index & 1) {
        // Odd pixel - low nibble
        display->buffer[byte_index] = (display->buffer[byte_index] & 0xF0) | (color & 0x0F);
    } else {
        // Even pixel - high nibble
        display->buffer[byte_index] = (display->buffer[byte_index] & 0x0F) | ((color & 0x0F) << 4);
    }
}

uint8_t inky_get_pixel(inky_t *display, uint16_t x, uint16_t y) {
    if (!display || !display->buffer) return 0;
    if (x >= display->width || y >= display->height) return 0;
    
    size_t pixel_index = y * display->width + x;
    size_t byte_index = pixel_index / 2;
    
    if (pixel_index & 1) {
        // Odd pixel - low nibble
        return display->buffer[byte_index] & 0x0F;
    } else {
        // Even pixel - high nibble
        return (display->buffer[byte_index] >> 4) & 0x0F;
    }
}

void inky_set_border(inky_t *display, uint8_t color) {
    if (!display) return;
    display->border_color = color & 0x07;
}

static void gpio_set_value(int fd, int line_index, int value) {
    struct gpiohandle_data data;
    data.values[line_index] = value;
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
    gpio_set_value(display->reset_line, 1, 0);  // DC is index 1
    
    // Set CS low
    gpio_set_value(display->reset_line, 2, 0);  // CS is index 2
    
    // Send command byte
    write(display->spi_fd, &command, 1);
    
    // Set CS high
    gpio_set_value(display->reset_line, 2, 1);
}

void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len) {
    if (!display || display->is_emulator || !data || len == 0) return;
    
    // Set DC high for data
    gpio_set_value(display->reset_line, 1, 1);  // DC is index 1
    
    // Set CS low
    gpio_set_value(display->reset_line, 2, 0);  // CS is index 2
    
    // Send data in chunks if needed
    const size_t chunk_size = 4096;
    size_t offset = 0;
    while (offset < len) {
        size_t to_send = (len - offset) > chunk_size ? chunk_size : (len - offset);
        write(display->spi_fd, data + offset, to_send);
        offset += to_send;
    }
    
    // Set CS high
    gpio_set_value(display->reset_line, 2, 1);
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
    gpio_set_value(display->reset_line, 0, 0);  // Reset low
    usleep(100000);  // 100ms
    gpio_set_value(display->reset_line, 0, 1);  // Reset high
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

void inky_update(inky_t *display) {
    if (!display) return;
    
    if (display->is_emulator) {
        // Handled in emulator
        return;
    }
    
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

// Stub for emulator function
int inky_emulator_save_ppm(inky_t *display, const char *filename) {
    (void)display;
    (void)filename;
    fprintf(stderr, "Cannot save PPM from hardware display\n");
    return -1;
}