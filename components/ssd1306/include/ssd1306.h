#pragma once

#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSD1306 OLED display dimensions
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64

// SSD1306 commands
#define SSD1306_CMD_SET_CONTRAST            0x81
#define SSD1306_CMD_DISPLAY_RAM             0xA4
#define SSD1306_CMD_DISPLAY_ALLON           0xA5
#define SSD1306_CMD_DISPLAY_NORMAL          0xA6
#define SSD1306_CMD_DISPLAY_INVERTED        0xA7
#define SSD1306_CMD_DISPLAY_OFF             0xAE
#define SSD1306_CMD_DISPLAY_ON              0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_CMD_SET_COM_PINS            0xDA
#define SSD1306_CMD_SET_COM_SCAN_MODE       0xC0
#define SSD1306_CMD_SET_COM_SCAN_DIR_DEC    0xC8  // Added COM scan direction decrement command
#define SSD1306_CMD_SET_DISPLAY_CLK_DIV     0xD5
#define SSD1306_CMD_SET_PRECHARGE           0xD9
#define SSD1306_CMD_SET_VCOM_DESELECT       0xDB
#define SSD1306_CMD_SET_MEMORY_ADDR_MODE    0x20
#define SSD1306_CMD_SET_COLUMN_ADDR         0x21
#define SSD1306_CMD_SET_PAGE_ADDR           0x22
#define SSD1306_CMD_SET_START_LINE          0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP       0xA0
#define SSD1306_CMD_SET_MULTIPLEX_RATIO     0xA8
#define SSD1306_CMD_CHARGE_PUMP             0x8D

// Display orientation modes
#define SSD1306_ORIENTATION_NORMAL          0 // Normal orientation
#define SSD1306_ORIENTATION_180_DEGREES     1 // Rotated 180 degrees

// SSD1306 handle type
typedef void* ssd1306_handle_t;

// Function declarations
ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t i2c_addr);
void ssd1306_delete(ssd1306_handle_t dev);
esp_err_t ssd1306_refresh_gram(ssd1306_handle_t dev);
esp_err_t ssd1306_clear_screen(ssd1306_handle_t dev, uint8_t chFill);
void ssd1306_set_position(ssd1306_handle_t dev, uint8_t page, uint8_t column);
void ssd1306_draw_pixel(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t color);
void ssd1306_fill_rectangle(ssd1306_handle_t dev, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void ssd1306_display_char(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t ch, uint8_t font_size, uint8_t mode);
void ssd1306_display_string(ssd1306_handle_t dev, uint8_t x, uint8_t y, const uint8_t *str, uint8_t font_size, uint8_t mode);

// New function to set display orientation
esp_err_t ssd1306_set_orientation(ssd1306_handle_t dev, uint8_t orientation);

#ifdef __cplusplus
}
#endif