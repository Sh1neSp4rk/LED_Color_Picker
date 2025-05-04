#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "ssd1306.h"

static const char *TAG = "SSD1306";

// Font data for characters (6x8 pixel font)
static const uint8_t font6x8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sp
    0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, // !
    0x00, 0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14, // #
    0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12, // $
    0x00, 0x62, 0x64, 0x08, 0x13, 0x23, // %
    0x00, 0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x00, 0x1c, 0x22, 0x41, 0x00, // (
    0x00, 0x00, 0x41, 0x22, 0x1c, 0x00, // )
    0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x00, 0x00, 0xA0, 0x60, 0x00, // ,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x00, 0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x00, 0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x00, 0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x00, 0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x00, 0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x00, 0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x00, 0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x00, 0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x00, 0x32, 0x49, 0x59, 0x51, 0x3E, // @
    0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C, // A
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x00, 0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x00, 0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x00, 0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x00, 0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55, // 55
    0x00, 0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x00, 0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x00, 0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x00, 0x01, 0x02, 0x04, 0x00, // '
    0x00, 0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x00, 0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x00, 0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x00, 0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x00, 0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x00, 0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C, // g
    0x00, 0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x00, 0x40, 0x80, 0x84, 0x7D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x00, 0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x00, 0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x00, 0xFC, 0x24, 0x24, 0x24, 0x18, // p
    0x00, 0x18, 0x24, 0x24, 0x18, 0xFC, // q
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x00, 0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x00, 0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x00, 0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C, // y
    0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x00, 0x08, 0x04, 0x08, 0x10, 0x08  // ~
};

// SSD1306 device structure
typedef struct {
    i2c_port_t i2c_port;    // I2C port number
    uint8_t i2c_addr;       // I2C device address
    uint8_t gram[SSD1306_HEIGHT/8][SSD1306_WIDTH]; // Graphics RAM (1 bit per pixel)
} ssd1306_dev_t;

// Write command to SSD1306
static esp_err_t ssd1306_write_cmd(ssd1306_handle_t dev, uint8_t cmd)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    uint8_t write_buf[2] = {0x00, cmd}; // Control byte (0x00) + command
    
    return i2c_master_write_to_device(device->i2c_port, device->i2c_addr, write_buf, sizeof(write_buf), 100 / portTICK_PERIOD_MS);
}

// Write data to SSD1306
static esp_err_t ssd1306_write_data(ssd1306_handle_t dev, uint8_t *data, size_t size)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    
    // Need 1 byte for control byte (0x40) + data size
    uint8_t *write_buf = malloc(size + 1);
    if (!write_buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for write buffer");
        return ESP_ERR_NO_MEM;
    }
    
    write_buf[0] = 0x40; // Control byte (0x40) for data
    memcpy(write_buf + 1, data, size);
    
    esp_err_t ret = i2c_master_write_to_device(device->i2c_port, device->i2c_addr, write_buf, size + 1, 100 / portTICK_PERIOD_MS);
    
    free(write_buf);
    return ret;
}

// Initialize the SSD1306 OLED display
static esp_err_t ssd1306_init(ssd1306_handle_t dev)
{
    esp_err_t ret;
    
    // Delay slightly before initialization
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Initialize display - basic initialization sequence for SSD1306
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_CLK_DIV);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x80); // Default value
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_MULTIPLEX_RATIO);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, SSD1306_HEIGHT - 1);
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_OFFSET);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x00); // No offset
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_START_LINE | 0x00); // Start line at 0
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_CHARGE_PUMP);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x14); // Enable charge pump
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_MEMORY_ADDR_MODE);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x00); // Horizontal addressing mode
    if (ret != ESP_OK) return ret;
    
    // These two commands control the display orientation
    // Set to flipped orientation (180 degrees) by default
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP | 0x01); // Flipped segment mapping (0xA1)
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_DIR_DEC); // Flipped COM scanning (0xC8)
    if (ret != ESP_OK) return ret;

    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_PINS);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x12); // COM pins configuration for 128x64
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_CONTRAST);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0xCF); // Contrast value
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_PRECHARGE);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0xF1); // Precharge period
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_VCOM_DESELECT);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0x40); // VCOM deselect level
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_RAM); // Display from RAM
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_NORMAL); // Normal display (not inverted)
    if (ret != ESP_OK) return ret;
    
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_ON); // Turn on display
    if (ret != ESP_OK) return ret;
    
    return ESP_OK;
}

// Create a new SSD1306 device instance
ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t i2c_addr)
{
    ssd1306_dev_t *dev = malloc(sizeof(ssd1306_dev_t));
    if (!dev) {
        ESP_LOGE(TAG, "Failed to allocate memory for SSD1306 device");
        return NULL;
    }
    
    // Initialize device structure
    dev->i2c_port = i2c_port;
    dev->i2c_addr = i2c_addr;
    memset(dev->gram, 0, sizeof(dev->gram));
    
    // Initialize display
    if (ssd1306_init((ssd1306_handle_t)dev) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SSD1306 display");
        free(dev);
        return NULL;
    }
    
    return (ssd1306_handle_t)dev;
}

// Delete SSD1306 device
void ssd1306_delete(ssd1306_handle_t dev)
{
    if (dev) {
        free(dev);
    }
}

// Refresh display with graphics RAM content
esp_err_t ssd1306_refresh_gram(ssd1306_handle_t dev)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    esp_err_t ret;
    
    // Set column address range
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_COLUMN_ADDR);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0); // Start column
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, SSD1306_WIDTH - 1); // End column
    if (ret != ESP_OK) return ret;
    
    // Set page address range
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_PAGE_ADDR);
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, 0); // Start page
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_cmd(dev, (SSD1306_HEIGHT / 8) - 1); // End page
    if (ret != ESP_OK) return ret;
    
    // Write entire display content
    return ssd1306_write_data(dev, (uint8_t *)device->gram, sizeof(device->gram));
}

// Clear screen with specified fill pattern
esp_err_t ssd1306_clear_screen(ssd1306_handle_t dev, uint8_t chFill)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    
    // Fill entire gram with pattern
    for (uint8_t page = 0; page < (SSD1306_HEIGHT / 8); page++) {
        for (uint8_t column = 0; column < SSD1306_WIDTH; column++) {
            device->gram[page][column] = chFill;
        }
    }
    
    return ssd1306_refresh_gram(dev);
}

// Set cursor position for drawing
void ssd1306_set_position(ssd1306_handle_t dev, uint8_t page, uint8_t column)
{
    ssd1306_write_cmd(dev, 0xB0 | page);                // Set page address
    ssd1306_write_cmd(dev, 0x00 | (column & 0x0F));     // Set column lower address
    ssd1306_write_cmd(dev, 0x10 | (column >> 4));       // Set column higher address
}

// Draw a single pixel
void ssd1306_draw_pixel(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t color)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return; // Out of bounds
    }
    
    uint8_t page = y / 8;
    uint8_t bit = 1 << (y % 8);
    
    if (color) {
        device->gram[page][x] |= bit;  // Set bit
    } else {
        device->gram[page][x] &= ~bit; // Clear bit
    }
}

// Fill a rectangle with color
void ssd1306_fill_rectangle(ssd1306_handle_t dev, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    for (uint8_t x = x1; x <= x2; x++) {
        for (uint8_t y = y1; y <= y2; y++) {
            ssd1306_draw_pixel(dev, x, y, color);
        }
    }
}

// Display a single character
void ssd1306_display_char(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t ch, uint8_t font_size, uint8_t mode)
{
    ssd1306_dev_t *device = (ssd1306_dev_t *)dev;
    
    // Only ASCII characters are supported
    if (ch < ' ' || ch > '~') {
        ch = '?';
    }
    
    // Get character's font data
    ch -= ' ';  // Offset from space character
    const uint8_t *font_data = &font6x8[ch * 6];
    
    for (uint8_t col = 0; col < 6; col++) {
        uint8_t font_col = font_data[col];
        
        for (uint8_t row = 0; row < 8; row++) {
            uint8_t pixel = (font_col & (1 << row)) ? 1 : 0;
            
            // Handle display mode (normal or inverted)
            if (mode) pixel = !pixel;
            
            // Handle font size
            if (font_size == 16) {  // Double size
                ssd1306_draw_pixel(dev, x + col*2, y + row*2, pixel);
                ssd1306_draw_pixel(dev, x + col*2 + 1, y + row*2, pixel);
                ssd1306_draw_pixel(dev, x + col*2, y + row*2 + 1, pixel);
                ssd1306_draw_pixel(dev, x + col*2 + 1, y + row*2 + 1, pixel);
            } else {  // Normal size
                ssd1306_draw_pixel(dev, x + col, y + row, pixel);
            }
        }
    }
}

// Display a string
void ssd1306_display_string(ssd1306_handle_t dev, uint8_t x, uint8_t y, const uint8_t *str, uint8_t font_size, uint8_t mode)
{
    uint8_t char_width = (font_size == 16) ? 12 : 6;  // Width for single or double size
    
    while (*str) {
        ssd1306_display_char(dev, x, y, *str, font_size, mode);
        x += char_width;
        
        // Handle string wrapping
        if (x > SSD1306_WIDTH - char_width) {
            x = 0;
            y += (font_size == 16) ? 16 : 8;
        }
        
        str++;
    }
}

// Set display orientation
esp_err_t ssd1306_set_orientation(ssd1306_handle_t dev, uint8_t orientation)
{
    esp_err_t ret;
    
    if (orientation == SSD1306_ORIENTATION_180_DEGREES) {
        // For 180-degree rotation (flipping the display)
        ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP | 0x01); // Flipped segment mapping (0xA1)
        if (ret != ESP_OK) return ret;
        
        ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_MODE | 0x08); // Flipped COM scan (0xC8)
        if (ret != ESP_OK) return ret;
    } else {
        // For normal orientation
        ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP); // Normal segment mapping (0xA0)
        if (ret != ESP_OK) return ret;
        
        ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_MODE); // Normal COM scan (0xC0)
        if (ret != ESP_OK) return ret;
    }
    
    return ESP_OK;
}