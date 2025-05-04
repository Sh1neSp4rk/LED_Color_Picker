#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "led_strip.h"
#include "driver/rmt.h"

static const char *TAG = "ws2812";

// WS2812 timing parameters (in microseconds)
#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (900)
#define WS2812_T1H_NS (900)
#define WS2812_T1L_NS (350)

// RMT clock period (in nanoseconds)
#define RMT_CLK_DURATION_NS (25) // 1/(80MHz/2) = 25ns

/**
 * @brief Convert RGB data to RMT format
 *
 * @note For WS2812, the order of color components is GRB
 */
static void IRAM_ATTR ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
                                         size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    
    const rmt_item32_t bit0 = {{{ (WS2812_T0H_NS / RMT_CLK_DURATION_NS), 1, (WS2812_T0L_NS / RMT_CLK_DURATION_NS), 0 }}};
    const rmt_item32_t bit1 = {{{ (WS2812_T1H_NS / RMT_CLK_DURATION_NS), 1, (WS2812_T1L_NS / RMT_CLK_DURATION_NS), 0 }}};
    
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    
    while (size < src_size && num < wanted_num) {
        uint8_t data = *psrc;
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (data & (1 << (7 - i))) {
                pdest->val = bit1.val;
            } else {
                pdest->val = bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

typedef struct {
    led_strip_t base;
    rmt_channel_t rmt_channel;
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;

static esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, base);
    if (index >= ws2812->strip_len) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // In the order of GRB
    uint32_t start = index * 3;
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;
    return ESP_OK;
}

static esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, base);
    esp_err_t ret = rmt_write_sample(ws2812->rmt_channel, ws2812->buffer, ws2812->strip_len * 3, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_write_sample failed");
        return ret;
    }
    return rmt_wait_tx_done(ws2812->rmt_channel, pdMS_TO_TICKS(timeout_ms));
}

static esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, base);
    // Write zero to all LEDs
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    return ws2812_refresh(strip, timeout_ms);
}

static esp_err_t ws2812_del(led_strip_t *strip)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, base);
    free(ws2812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config)
{
    if (!config || config->max_leds == 0) {
        ESP_LOGE(TAG, "Invalid arguments");
        return NULL;
    }
    
    // Allocate memory for the strip (header + data buffer)
    uint32_t strip_len = config->max_leds;
    ws2812_t *ws2812 = calloc(1, sizeof(ws2812_t) + strip_len * 3);
    if (!ws2812) {
        ESP_LOGE(TAG, "Failed to allocate memory for led_strip");
        return NULL;
    }
    
    // Configure RMT translator
    rmt_translator_init((rmt_channel_t)config->dev, ws2812_rmt_adapter);
    
    // Fill in function pointers
    ws2812->base.set_pixel = ws2812_set_pixel;
    ws2812->base.refresh = ws2812_refresh;
    ws2812->base.clear = ws2812_clear;
    ws2812->base.del = ws2812_del;
    
    // Save parameters
    ws2812->rmt_channel = (rmt_channel_t)config->dev;
    ws2812->strip_len = strip_len;
    
    return &ws2812->base;
}