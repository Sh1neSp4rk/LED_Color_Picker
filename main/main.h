#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Debug tag for logging
#define TAG "LED_COLOR_PICKER"

// GPIO pin definitions
#define RGB_LED_DATA_PIN     1   // GPIO1 for RGB LED data
#define ONBOARD_LED_PIN      21  // GPIO21 for onboard RGB LED
#define BOOT_BUTTON_PIN      0   // GPIO0 for boot button

#define RED_POT_ADC_CHANNEL   ADC_CHANNEL_1
#define GREEN_POT_ADC_CHANNEL ADC_CHANNEL_2
#define BLUE_POT_ADC_CHANNEL  ADC_CHANNEL_3

// I2C pins for OLED display
#define OLED_SDA_PIN         5
#define OLED_SCL_PIN         6
#define I2C_MASTER_FREQ_HZ   400000  // I2C clock frequency
#define OLED_ADDR            0x78    // Updated address for SSD1306 (was 0x3C)

// I2C master number
#define I2C_MASTER_NUM       I2C_NUM_0

// RGB LED parameters
#define LED_COUNT            4      // 4 RGB LEDs 
#define LED_BRIGHTNESS       64     // 0-255 (lower is brighter for WS2812)
#define DEBOUNCE_TIME_MS     200    // Debounce time for button in milliseconds

// Function prototypes
void app_main(void);
void init_gpio(void);
void init_i2c(void);
bool init_adc(adc_oneshot_unit_handle_t *adc1_handle);
void init_rgb_leds(void);
void update_rgb_leds(uint8_t red, uint8_t green, uint8_t blue);
void update_onboard_led(uint8_t red, uint8_t green, uint8_t blue);
void init_oled(void);
void update_oled_display(uint8_t red, uint8_t green, uint8_t blue);
uint8_t read_potentiometer(adc_oneshot_unit_handle_t adc1_handle, adc_channel_t channel);
void button_task(void *pvParameter);
void debug_adc_values(adc_oneshot_unit_handle_t adc1_handle);

#endif // MAIN_H