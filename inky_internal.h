#ifndef INKY_INTERNAL_H
#define INKY_INTERNAL_H

#include "inky.h"

// UC8159 Command Set
#define UC8159_PSR   0x00  // Panel Setting
#define UC8159_PWR   0x01  // Power Setting
#define UC8159_POF   0x02  // Power OFF
#define UC8159_PFS   0x03  // Power OFF Sequence
#define UC8159_PON   0x04  // Power ON
#define UC8159_BTST  0x06  // Booster Soft Start
#define UC8159_DSLP  0x07  // Deep Sleep
#define UC8159_DTM1  0x10  // Data Start Transmission 1
#define UC8159_DSP   0x11  // Data Stop
#define UC8159_DRF   0x12  // Display Refresh
#define UC8159_IPC   0x13  // Image Process Command
#define UC8159_PLL   0x30  // PLL Control
#define UC8159_TSC   0x40  // Temperature Sensor Calibration
#define UC8159_TSE   0x41  // Temperature Sensor Enable
#define UC8159_TSW   0x42  // Temperature Sensor Write
#define UC8159_TSR   0x43  // Temperature Sensor Read
#define UC8159_CDI   0x50  // VCOM and Data Interval Setting
#define UC8159_LPD   0x51  // Low Power Detection
#define UC8159_TCON  0x60  // TCON Setting
#define UC8159_TRES  0x61  // Resolution Setting
#define UC8159_DAM   0x65  // Data Mode
#define UC8159_REV   0x70  // Revision
#define UC8159_FLG   0x71  // Get Status
#define UC8159_AMV   0x80  // Auto Measurement VCOM
#define UC8159_VV    0x81  // VCOM Value
#define UC8159_VDCS  0x82  // VCOM DC Setting
#define UC8159_PWS   0xE3  // Power Saving
#define UC8159_TSSET 0xE5  // Temperature Sensor Selection

// GPIO Pin definitions (Raspberry Pi BCM numbering)
#define INKY_RESET_PIN 27  // BCM27 (Physical Pin 13)
#define INKY_BUSY_PIN  17  // BCM17 (Physical Pin 11)
#define INKY_DC_PIN    22  // BCM22 (Physical Pin 15)
#define INKY_CS_PIN    8   // BCM8/CE0 (Physical Pin 24)

// Button GPIO Pin definitions (Raspberry Pi BCM numbering)
#define INKY_BUTTON_A_PIN 5   // BCM5 (Physical Pin 29)
#define INKY_BUTTON_B_PIN 6   // BCM6 (Physical Pin 31)
#define INKY_BUTTON_C_PIN 16  // BCM16 (Physical Pin 36)
#define INKY_BUTTON_D_PIN 24  // BCM24 (Physical Pin 18)

// Internal display structure (implementation exposed to backends)
struct inky_display {
    // Display properties
    uint16_t width;
    uint16_t height;
    uint8_t border_color;
    
    // Display buffer - packed 4-bit pixels (2 pixels per byte)
    uint8_t *buffer;
    size_t buffer_size;
    
    // Backend type
    bool is_emulator;
    
    // Hardware specific (only used when !is_emulator)
    int spi_fd;
    int gpio_chip_fd;
    int reset_line;
    int busy_line;
    int dc_line;
    int cs_line;
    
    // Flip settings
    bool h_flip;
    bool v_flip;
};

// Common functions (shared between emulator and hardware)
inky_t* inky_init_common(bool emulator);
void inky_destroy_common(inky_t *display);

// Hardware-specific internal functions
bool inky_hw_init_gpio(inky_t *display);
void inky_hw_setup(inky_t *display);
void inky_hw_reset(inky_t *display);
void inky_hw_send_command(inky_t *display, uint8_t command);
void inky_hw_send_data(inky_t *display, const uint8_t *data, size_t len);
void inky_hw_busy_wait(inky_t *display);
void inky_hw_update(inky_t *display);

#endif // INKY_INTERNAL_H