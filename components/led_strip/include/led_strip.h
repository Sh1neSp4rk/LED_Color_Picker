#pragma once

#include <driver/rmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED Strip Type
 */
typedef struct led_strip_s led_strip_t;

/**
 * @brief LED Strip Device Type
 */
typedef void *led_strip_dev_t;

/**
 * @brief LED Strip Configuration Type
 */
typedef struct {
    uint32_t max_leds;       /*!< Maximum number of LEDs in the strip */
    led_strip_dev_t dev;     /*!< LED strip device (e.g. RMT channel or SPI device) */
} led_strip_config_t;

/**
 * @brief Default LED Strip Configuration
 */
#define LED_STRIP_DEFAULT_CONFIG(number_of_leds, dev_handle) \
    {                                                 \
        .max_leds = number_of_leds,                   \
        .dev = dev_handle,                           \
    }

/**
 * @brief LED Strip interface
 */
struct led_strip_s {
    /**
     * @brief Set RGB for a specific pixel
     *
     * @param strip: LED strip
     * @param index: Index of pixel to set
     * @param red: Red component
     * @param green: Green component
     * @param blue: Blue component
     *
     * @return
     *      - ESP_OK: Set RGB for a specific pixel successfully
     *      - ESP_ERR_INVALID_ARG: Set RGB for a specific pixel failed because of invalid parameters
     *      - ESP_FAIL: Set RGB for a specific pixel failed because other error occurred
     */
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

    /**
     * @brief Refresh memory colors to LEDs
     *
     * @param strip: LED strip
     * @param timeout_ms: Timeout value for refreshing task
     *
     * @return
     *      - ESP_OK: Refresh successfully
     *      - ESP_ERR_TIMEOUT: Refresh failed because of timeout
     *      - ESP_FAIL: Refresh failed because some other error occurred
     */
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);

    /**
     * @brief Clear LED strip (turn off all LEDs)
     *
     * @param strip: LED strip
     * @param timeout_ms: Timeout value for clearing task
     *
     * @return
     *      - ESP_OK: Clear LEDs successfully
     *      - ESP_ERR_TIMEOUT: Clear LEDs failed because of timeout
     *      - ESP_FAIL: Clear LEDs failed because some other error occurred
     */
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);

    /**
     * @brief Free LED strip resources
     *
     * @param strip: LED strip
     *
     * @return
     *      - ESP_OK: Free resources successfully
     *      - ESP_FAIL: Free resources failed because error occurred
     */
    esp_err_t (*del)(led_strip_t *strip);
};

/**
 * @brief Create LED strip based on WS2812 driver (RMT peripheral)
 *
 * @param config: LED strip configuration
 * @return
 *      LED strip instance or NULL
 */
led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);

#ifdef __cplusplus
}
#endif