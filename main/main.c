#include "main.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "freertos/queue.h"

// For SSD1306 OLED display
#include "ssd1306.h"

static led_strip_t *strip;
static led_strip_t *onboard_led;
static ssd1306_handle_t ssd1306_dev = NULL;

// Button state variables
static bool onboard_led_active = false;
static QueueHandle_t gpio_evt_queue = NULL;

// Initialize GPIO pins
void init_gpio(void)
{
    gpio_reset_pin(RGB_LED_DATA_PIN);
    gpio_set_direction(RGB_LED_DATA_PIN, GPIO_MODE_OUTPUT);
    
    // Configure BOOT button as input with pull-up
    gpio_reset_pin(BOOT_BUTTON_PIN);
    gpio_set_direction(BOOT_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOOT_BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_intr_enable(BOOT_BUTTON_PIN);
    
    // Configure GPIO interrupt for button
    gpio_install_isr_service(0);
    gpio_set_intr_type(BOOT_BUTTON_PIN, GPIO_INTR_NEGEDGE);
    
    // Create a queue to handle GPIO event from ISR
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    ESP_LOGI(TAG, "GPIO initialized");
}

// GPIO interrupt handler
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Button task to handle button presses
void button_task(void *pvParameter)
{
    uint32_t gpio_num;
    TickType_t last_press_time = 0;
    
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            // Simple debounce
            TickType_t current_time = xTaskGetTickCount();
            if((current_time - last_press_time) < pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
                continue; // Skip this press (debounce)
            }
            last_press_time = current_time;
            
            // Toggle onboard LED state
            if(gpio_num == BOOT_BUTTON_PIN) {
                onboard_led_active = !onboard_led_active;
                ESP_LOGI(TAG, "Boot button pressed, onboard LED %s", onboard_led_active ? "ON" : "OFF");
                
                if(onboard_led_active) {
                    // Turn on with current RGB value
                    ESP_ERROR_CHECK(onboard_led->set_pixel(onboard_led, 0, 255, 255, 255));
                } else {
                    // Turn off
                    ESP_ERROR_CHECK(onboard_led->clear(onboard_led, 100));
                }
                ESP_ERROR_CHECK(onboard_led->refresh(onboard_led, 100));
            }
        }
    }
}

// Initialize I2C
void init_i2c(void)
{
    // First explicitly set SDA and SCL pins as GPIO
    gpio_reset_pin(OLED_SDA_PIN);
    gpio_reset_pin(OLED_SCL_PIN);
    gpio_set_direction(OLED_SDA_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(OLED_SCL_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(OLED_SDA_PIN, GPIO_PULLUP_ENABLE);
    gpio_set_pull_mode(OLED_SCL_PIN, GPIO_PULLUP_ENABLE);
    
    // Now configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_PIN,
        .scl_io_num = OLED_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C initialized with SDA:%d, SCL:%d", OLED_SDA_PIN, OLED_SCL_PIN);
}

// Initialize ADC for potentiometers
bool init_adc(adc_oneshot_unit_handle_t *adc1_handle)
{
    // ADC configuration
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc1_handle));
    
    // ADC channel configuration
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12, // Use 12dB attenuation for full 3.3V range
    };
    
    // ESP32-S3 ADC channel to GPIO mapping:
    // ADC1 Channel 0 -> GPIO1
    // ADC1 Channel 1 -> GPIO2 (now confirmed as red pot)
    // ADC1 Channel 2 -> GPIO3 (now confirmed as blue pot)
    // ADC1 Channel 3 -> GPIO4 (now confirmed as green pot)
    // etc.
    
    // Using proper ADC1 channel enumeration for ESP32-S3 with corrected pot assignments
    ESP_LOGI(TAG, "Configuring ADC channels with correct mappings:");
    ESP_LOGI(TAG, "Red: CH%d (GPIO2), Green: CH%d (GPIO4), Blue: CH%d (GPIO3)",
             RED_POT_ADC_CHANNEL, GREEN_POT_ADC_CHANNEL, BLUE_POT_ADC_CHANNEL);
    
    // Configure channels for potentiometers using the corrected mappings from main.h
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle, RED_POT_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle, GREEN_POT_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle, BLUE_POT_ADC_CHANNEL, &config));
    
    ESP_LOGI(TAG, "ADC initialized with corrected potentiometer mappings");
    return true;
}

// Test multiple ADC channels to find which one responds to the blue pot
uint8_t find_blue_pot_channel(adc_oneshot_unit_handle_t adc1_handle)
{
    // Array of possible ADC channels to test for blue pot
    adc_channel_t test_channels[] = {
        ADC_CHANNEL_0,  // GPIO1
        ADC_CHANNEL_3,  // GPIO4
        ADC_CHANNEL_4,  // GPIO5
        ADC_CHANNEL_5,  // GPIO6
        ADC_CHANNEL_6,  // GPIO7
        ADC_CHANNEL_7,  // GPIO8
        ADC_CHANNEL_8   // GPIO9
    };
    const int num_channels = sizeof(test_channels) / sizeof(test_channels[0]);
    
    int readings[num_channels];
    int prev_readings[num_channels];
    
    // Initialize previous readings
    for (int i = 0; i < num_channels; i++) {
        adc_oneshot_read(adc1_handle, test_channels[i], &prev_readings[i]);
    }
    
    // Print headers for debug output
    ESP_LOGI(TAG, "Testing multiple ADC channels to find blue pot:");
    char header[100];
    sprintf(header, "Channel: ");
    for (int i = 0; i < num_channels; i++) {
        sprintf(header + strlen(header), " CH%d", test_channels[i]);
    }
    ESP_LOGI(TAG, "%s", header);
    
    // Test for 10 iterations to see which channel changes when potentiometer is adjusted
    for (int iter = 0; iter < 10; iter++) {
        // Read all channels
        for (int i = 0; i < num_channels; i++) {
            adc_oneshot_read(adc1_handle, test_channels[i], &readings[i]);
        }
        
        // Format and print values
        char values[100];
        sprintf(values, "Values:  ");
        for (int i = 0; i < num_channels; i++) {
            sprintf(values + strlen(values), " %4d", readings[i]);
        }
        ESP_LOGI(TAG, "%s", values);
        
        // Check for significant changes
        char changes[100];
        sprintf(changes, "Changes: ");
        for (int i = 0; i < num_channels; i++) {
            int diff = readings[i] - prev_readings[i];
            sprintf(changes + strlen(changes), " %4d", diff);
            
            // If we detect a significant change, this might be our blue pot
            if (abs(diff) > 500) {
                ESP_LOGI(TAG, "Significant change on ADC_CHANNEL_%d, might be blue pot!", test_channels[i]);
            }
            
            prev_readings[i] = readings[i];
        }
        ESP_LOGI(TAG, "%s", changes);
        
        // Wait a bit before next reading
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    // Default to channel 6 for now, but you should check the logs and update this
    return ADC_CHANNEL_6;  // This will be updated based on test results
}

// Initialize RGB LEDs (using RMT peripheral and WS2812 driver)
void init_rgb_leds(void)
{
    // Main RGB LED strip
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(RGB_LED_DATA_PIN, 0);
    // Set suitable clock divider
    config.clk_div = 2;
    
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    
    // Install led strip driver for main LEDs
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(LED_COUNT, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    
    if (!strip) {
        ESP_LOGE(TAG, "Install main LED strip failed");
        return;
    }
    
    // Set all LEDs to initial value (off)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    
    // Onboard RGB LED
    rmt_config_t onboard_config = RMT_DEFAULT_CONFIG_TX(ONBOARD_LED_PIN, 1);
    onboard_config.clk_div = 2;
    
    ESP_ERROR_CHECK(rmt_config(&onboard_config));
    ESP_ERROR_CHECK(rmt_driver_install(onboard_config.channel, 0, 0));
    
    // Install led strip driver for onboard LED
    led_strip_config_t onboard_strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)onboard_config.channel);
    onboard_led = led_strip_new_rmt_ws2812(&onboard_strip_config);
    
    if (!onboard_led) {
        ESP_LOGE(TAG, "Install onboard LED failed");
        return;
    }
    
    // Set onboard LED to initial value (off)
    ESP_ERROR_CHECK(onboard_led->clear(onboard_led, 100));
    
    // Configure ISR for button
    gpio_isr_handler_add(BOOT_BUTTON_PIN, gpio_isr_handler, (void*) BOOT_BUTTON_PIN);
    
    ESP_LOGI(TAG, "RGB LEDs initialized");
}

// Update the RGB LEDs with new color values
void update_rgb_leds(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int i = 0; i < LED_COUNT; i++) {
        ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
    }
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
}

// Update the onboard LED with new color values (if active)
void update_onboard_led(uint8_t red, uint8_t green, uint8_t blue)
{
    if (onboard_led_active) {
        ESP_ERROR_CHECK(onboard_led->set_pixel(onboard_led, 0, red, green, blue));
        ESP_ERROR_CHECK(onboard_led->refresh(onboard_led, 100));
    }
}

// Initialize OLED display
void init_oled(void)
{
    ESP_LOGI(TAG, "Initializing OLED display at address 0x%02X", OLED_ADDR);
    
    // First, try to verify the I2C device is present with a direct scan
    uint8_t device_count = 0;
    for (uint8_t i = 1; i < 128; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found I2C device at address 0x%02X", i);
            device_count++;
            
            if (i == (OLED_ADDR >> 1)) {  // Convert 8-bit to 7-bit address if needed
                ESP_LOGI(TAG, "Found OLED display at address 0x%02X", i);
            }
        }
    }
    
    if (device_count == 0) {
        ESP_LOGE(TAG, "No I2C devices found! Check connections");
        return;
    }
    
    // Give some time for the display to initialize its internal circuits
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Try to initialize the display with the configured address
    ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, OLED_ADDR);
    if (ssd1306_dev == NULL) {
        ESP_LOGE(TAG, "OLED display initialization failed at address 0x%02X", OLED_ADDR);
        
        // Try alternate address format (convert between 7-bit and 8-bit)
        uint8_t alt_addr = ((OLED_ADDR & 0x80) == 0) ? OLED_ADDR >> 1 : OLED_ADDR;
        ESP_LOGI(TAG, "Trying alternate address format: 0x%02X", alt_addr);
        
        ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, alt_addr);
        if (ssd1306_dev == NULL) {
            ESP_LOGE(TAG, "OLED display initialization failed with alternate address");
            return;
        }
    }
    
    // Fix the display orientation by setting it to 180 degrees
    if (ssd1306_set_orientation(ssd1306_dev, SSD1306_ORIENTATION_180_DEGREES) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set OLED display orientation");
    } else {
        ESP_LOGI(TAG, "OLED display orientation set to 180 degrees");
    }
    
    // If we got here, the display was initialized successfully
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    
    // Draw initialization screen
    ssd1306_display_string(ssd1306_dev, 0, 0, (uint8_t *)"LED Color Picker", 16, 0);
    ssd1306_display_string(ssd1306_dev, 0, 20, (uint8_t *)"Initialized!", 16, 0);
    ssd1306_refresh_gram(ssd1306_dev);
    
    // Short delay to show init screen
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    ESP_LOGI(TAG, "OLED initialized successfully");
}

// Update OLED display with new color values
void update_oled_display(uint8_t red, uint8_t green, uint8_t blue)
{
    // Skip updating if the display was not initialized properly
    if (ssd1306_dev == NULL) {
        return;
    }
    
    char red_str[20], green_str[20], blue_str[20], rgb_str[20];
    
    // Clear previous data
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    
    // Format and display RGB values
    snprintf(red_str, sizeof(red_str), "R: %3d", red);
    snprintf(green_str, sizeof(green_str), "G: %3d", green);
    snprintf(blue_str, sizeof(blue_str), "B: %3d", blue);
    snprintf(rgb_str, sizeof(rgb_str), "#%02X%02X%02X", red, green, blue);
    
    // Title
    ssd1306_display_string(ssd1306_dev, 0, 0, (uint8_t *)"LED Color Picker", 16, 0);
    
    // RGB value in hex
    ssd1306_display_string(ssd1306_dev, 0, 16, (uint8_t *)rgb_str, 16, 0);
    
    // Individual RGB component values - Now using the formatted strings with actual values
    ssd1306_display_string(ssd1306_dev, 0, 32, (uint8_t *)red_str, 16, 0);
    ssd1306_display_string(ssd1306_dev, 42, 32, (uint8_t *)green_str, 16, 0);
    ssd1306_display_string(ssd1306_dev, 84, 32, (uint8_t *)blue_str, 16, 0);
    
    // Draw color preview box
    ssd1306_fill_rectangle(ssd1306_dev, 0, 48, 127, 63, 1);
    
    // Create a visual bar graph for each RGB component
    // Draw three bar graphs showing R, G, B levels
    
    // Red level bar (first row of bar graph)
    uint8_t bar_width = (red * 40) / 255;
    ssd1306_fill_rectangle(ssd1306_dev, 5, 52, 5 + bar_width, 54, 0);
    
    // Green level bar (second row of bar graph)
    bar_width = (green * 40) / 255;
    ssd1306_fill_rectangle(ssd1306_dev, 45, 52, 45 + bar_width, 54, 0);
    
    // Blue level bar (third row of bar graph)
    bar_width = (blue * 40) / 255;
    ssd1306_fill_rectangle(ssd1306_dev, 85, 52, 85 + bar_width, 54, 0);
    
    // Refresh the display to show all changes
    ssd1306_refresh_gram(ssd1306_dev);
}

// Read ADC value from potentiometer and convert to 0-255 range
uint8_t read_potentiometer(adc_oneshot_unit_handle_t adc1_handle, adc_channel_t channel)
{
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));
    
    // Convert directly from ADC range (0-4095) to 0-255 without reversing
    // This makes clockwise rotation increase values
    return (uint8_t)((adc_raw * 255) / 4095);
}

// Debug function to print raw ADC values with more detail
void debug_adc_values(adc_oneshot_unit_handle_t adc1_handle)
{
    int adc_raw[8]; // Array to hold readings from multiple channels
    
    // Read from multiple potential channels (0-7)
    for (int i = 0; i < 8; i++) {
        // Use safe error handling for each read
        esp_err_t ret = adc_oneshot_read(adc1_handle, i, &adc_raw[i]);
        if (ret != ESP_OK) {
            adc_raw[i] = -1; // Mark as invalid reading
        }
    }
    
    // Display all values in a readable format
    ESP_LOGI(TAG, "Raw ADC values per channel:");
    ESP_LOGI(TAG, "CH0: %4d | CH1: %4d | CH2: %4d | CH3: %4d", 
             adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3]);
    ESP_LOGI(TAG, "CH4: %4d | CH5: %4d | CH6: %4d | CH7: %4d", 
             adc_raw[4], adc_raw[5], adc_raw[6], adc_raw[7]);
    
    ESP_LOGI(TAG, "Current mapping - Red: CH%d (%d), Green: CH%d (%d), Blue: CH%d (%d)",
             RED_POT_ADC_CHANNEL, adc_raw[RED_POT_ADC_CHANNEL], 
             GREEN_POT_ADC_CHANNEL, adc_raw[GREEN_POT_ADC_CHANNEL], 
             BLUE_POT_ADC_CHANNEL, adc_raw[BLUE_POT_ADC_CHANNEL]);
}

void app_main(void)
{
    // Initialize components
    ESP_LOGI(TAG, "Starting LED Color Picker");
    
    init_gpio();
    init_i2c();
    
    adc_oneshot_unit_handle_t adc1_handle;
    if (!init_adc(&adc1_handle)) {
        ESP_LOGE(TAG, "Failed to initialize ADC");
        return;
    }
    
    init_rgb_leds();
    init_oled();
    
    // Run the blue pot detection routine
    ESP_LOGI(TAG, "Starting automatic detection of blue potentiometer channel...");
    // Uncomment to run the detection routine:
    // uint8_t detected_blue_channel = find_blue_pot_channel(adc1_handle);
    // ESP_LOGI(TAG, "Detection suggests blue potentiometer might be on channel: %d", detected_blue_channel);
    
    // Create a task to handle button presses
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    
    // Main loop
    uint8_t red = 0, green = 0, blue = 0;
    uint8_t prev_red = 0, prev_green = 0, prev_blue = 0;
    uint32_t debug_counter = 0;
    
    // Try different channels for blue pot
    adc_channel_t blue_channel_options[] = {
        ADC_CHANNEL_0,  // Try GPIO1
        ADC_CHANNEL_3,  // Try GPIO4
        ADC_CHANNEL_4,  // Try GPIO5
        ADC_CHANNEL_5,  // Try GPIO6
        ADC_CHANNEL_6,  // Try GPIO7
    };
    int current_channel_index = 0;
    int num_channels = sizeof(blue_channel_options) / sizeof(blue_channel_options[0]);
    adc_channel_t current_blue_channel = BLUE_POT_ADC_CHANNEL; // Use the defined value from main.h instead of auto-rotating
    
    // Automatically rotate through potential blue pot channels
    uint32_t channel_switch_counter = 0;
    bool auto_rotate_channels = false; // Disable auto-rotation of channels
    
    ESP_LOGI(TAG, "Entering main loop - using channel %d for blue pot", BLUE_POT_ADC_CHANNEL);
    while (1) {
        // Print debug ADC values every 20 iterations
        if (debug_counter % 20 == 0) {
            debug_adc_values(adc1_handle);
            
            // Rotate through channels automatically every ~5 seconds (if enabled)
            if (auto_rotate_channels && ++channel_switch_counter >= 100) {
                channel_switch_counter = 0;
                current_channel_index = (current_channel_index + 1) % num_channels;
                current_blue_channel = blue_channel_options[current_channel_index];
                ESP_LOGI(TAG, "Switching blue pot to try channel %d", current_blue_channel);
            }
        }
        debug_counter++;
        
        // Read potentiometer values
        red = read_potentiometer(adc1_handle, RED_POT_ADC_CHANNEL);
        green = read_potentiometer(adc1_handle, GREEN_POT_ADC_CHANNEL);
        blue = read_potentiometer(adc1_handle, BLUE_POT_ADC_CHANNEL);  // Use the fixed channel from main.h, not the current_blue_channel variable
        
        // Update only if the color has changed
        if (red != prev_red || green != prev_green || blue != prev_blue) {
            update_rgb_leds(red, green, blue);
            update_oled_display(red, green, blue);
            update_onboard_led(red, green, blue);
            
            if (debug_counter % 10 == 0) {
                ESP_LOGI(TAG, "Color updated: R=%d (CH%d), G=%d (CH%d), B=%d (CH%d)", 
                       red, RED_POT_ADC_CHANNEL,
                       green, GREEN_POT_ADC_CHANNEL,
                       blue, BLUE_POT_ADC_CHANNEL);
            }
            
            prev_red = red;
            prev_green = green;
            prev_blue = blue;
        }
        
        // Small delay to avoid excessive updates
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}