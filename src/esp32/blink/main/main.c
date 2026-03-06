#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"

#define LED_GPIO 48
#define LED_STRIP_LED_COUNT 1

static const char *TAG = "blink";
static led_strip_handle_t led_strip;

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing addressable LED on GPIO%d", LED_GPIO);
    
    // Configure LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_STRIP_LED_COUNT,
    };
    
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // Clear LED strip (turn off)
    led_strip_clear(led_strip);
    
    uint8_t brightness = 10; // Low brightness to start
    
    while (1) {
        ESP_LOGI(TAG, "LED ON - Red");
        led_strip_set_pixel(led_strip, 0, brightness, 0, 0); // Red
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "LED OFF");
        led_strip_clear(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}