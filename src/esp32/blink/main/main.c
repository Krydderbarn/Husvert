/*
 * Husvert - Distributed Home Monitoring and Control System
 * Phase 1.2: WiFi Connectivity Test
 * 
 * This version tests WiFi connection before adding LED status indicators.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/gpio.h"

// WiFi module
#include "wifi_app.h"

// Logging tag
static const char *TAG = "main";

// LED configuration
#define LED_STRIP_GPIO 48
#define LED_STRIP_MAX_LEDS 1

// Global LED strip handle
static led_strip_handle_t led_strip;

/**
 * @brief Configure and initialize the RGB LED
 */
static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring addressable LED on GPIO %d", LED_STRIP_GPIO);
    
    // LED strip general configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = LED_STRIP_MAX_LEDS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812 format
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    
    // LED strip RMT backend configuration
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  // 10MHz for precise timing
        .flags.with_dma = false,
    };
    
    // Create LED strip object
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // Clear LED on startup (turn off)
    led_strip_clear(led_strip);
}

/**
 * @brief Set LED to a specific color
 * 
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 */
static void set_led_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Husvert Phase 1.2 - WiFi Connectivity Test");
    ESP_LOGI(TAG, "====================================");
    
    // Step 1: Configure LED
    configure_led();
    
    // Step 2: Show startup (blue LED)
    ESP_LOGI(TAG, "Setting LED to BLUE (initializing...)");
    set_led_color(0, 0, 10);  // Blue
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Step 3: Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    esp_err_t ret = wifi_init_sta();
    
    // Step 4: Check result and set LED accordingly
    if (ret == ESP_OK) {
        // Success - Green LED
        ESP_LOGI(TAG, "✓ WiFi connected successfully!");
        ESP_LOGI(TAG, "Setting LED to GREEN (connected)");
        set_led_color(0, 10, 0);  // Green
    } else {
        // Failed - Red LED
        ESP_LOGE(TAG, "✗ WiFi connection failed!");
        ESP_LOGI(TAG, "Setting LED to RED (failed)");
        set_led_color(10, 0, 0);  // Red
    }
    
    // Step 5: Main loop - just blink the LED slowly to show we're alive
    ESP_LOGI(TAG, "Entering main loop...");
    bool led_on = true;
    
    while (1) {
        // Check WiFi status periodically
        if (wifi_is_connected()) {
            // Blink green slowly (heartbeat)
            if (led_on) {
                set_led_color(0, 10, 0);  // Green on
            } else {
                set_led_color(0, 2, 0);   // Green dim
            }
        } else {
            // Blink red if disconnected
            if (led_on) {
                set_led_color(10, 0, 0);  // Red on
            } else {
                set_led_color(2, 0, 0);   // Red dim
            }
        }
        
        led_on = !led_on;
        vTaskDelay(pdMS_TO_TICKS(2000));  // 2 second blink
    }
}