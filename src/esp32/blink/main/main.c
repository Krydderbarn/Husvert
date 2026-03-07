/*
 * Husvert - Distributed Home Monitoring and Control System
 * Phase 1.4: Sensor Integration (Event-Driven Architecture)
 * 
 * Features:
 * - WiFi connectivity
 * - MQTT publish/subscribe
 * - Remote LED control via MQTT
 * - Temperature & humidity monitoring
 * 
 * Architecture:
 * - main.c is the application hub (orchestration layer)
 * - Modules fire callbacks/events, main.c routes them
 * - Loose coupling: modules don't know about each other
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/gpio.h"

// Our modules
#include "wifi_app.h"
#include "mqtt_app.h"
#include "mqtt_config.h"  // For topic names
#include "sensor_app.h"

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
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    
    // LED strip RMT backend configuration
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    
    // Create LED strip object
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // Clear LED on startup
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

/**
 * @brief Handle LED commands from MQTT
 * 
 * This function is called by mqtt_app.c when a command arrives.
 * It's registered as a callback via mqtt_set_led_callback().
 * 
 * @param command LED command string ("red", "green", "blue", "yellow", "off")
 */
void handle_led_command(const char *command)
{
    ESP_LOGI(TAG, "LED command received: %s", command);
    
    if (strcmp(command, "red") == 0) {
        set_led_color(10, 0, 0);
        ESP_LOGI(TAG, "LED set to RED");
        
    } else if (strcmp(command, "green") == 0) {
        set_led_color(0, 10, 0);
        ESP_LOGI(TAG, "LED set to GREEN");
        
    } else if (strcmp(command, "blue") == 0) {
        set_led_color(0, 0, 10);
        ESP_LOGI(TAG, "LED set to BLUE");
        
    } else if (strcmp(command, "yellow") == 0) {
        set_led_color(10, 10, 0);
        ESP_LOGI(TAG, "LED set to YELLOW");
        
    } else if (strcmp(command, "purple") == 0) {
        set_led_color(10, 0, 10);
        ESP_LOGI(TAG, "LED set to PURPLE");
        
    } else if (strcmp(command, "cyan") == 0) {
        set_led_color(0, 10, 10);
        ESP_LOGI(TAG, "LED set to CYAN");
        
    } else if (strcmp(command, "white") == 0) {
        set_led_color(10, 10, 10);
        ESP_LOGI(TAG, "LED set to WHITE");
        
    } else if (strcmp(command, "off") == 0) {
        set_led_color(0, 0, 0);
        ESP_LOGI(TAG, "LED turned OFF");
        
    } else {
        ESP_LOGW(TAG, "Unknown LED command: %s", command);
    }
}

/**
 * @brief Handle sensor data events
 * 
 * This is the EVENT HANDLER for sensor data.
 * Called by sensor_app.c whenever new data is available.
 * 
 * @param temperature Temperature in °C
 * @param humidity Relative humidity in %
 */
void handle_sensor_data(float temperature, float humidity)
{
    ESP_LOGI(TAG, "Sensor event received: %.2f °C, %.2f %%", temperature, humidity);
    
    // Convert to strings
    char temp_str[16];
    char hum_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.2f", temperature);
    snprintf(hum_str, sizeof(hum_str), "%.2f", humidity);
    
    // Publish temperature
    ESP_LOGI(TAG, "Publishing %s: %s °C", MQTT_TOPIC_TEMPERATURE, temp_str);
    int temp_id = mqtt_publish(MQTT_TOPIC_TEMPERATURE, temp_str, 0, 1);
    
    // Publish humidity
    ESP_LOGI(TAG, "Publishing %s: %s %%", MQTT_TOPIC_HUMIDITY, hum_str);
    int hum_id = mqtt_publish(MQTT_TOPIC_HUMIDITY, hum_str, 0, 1);
    
    // Check results
    if (temp_id >= 0 && hum_id >= 0) {
        ESP_LOGI(TAG, "Sensor data published successfully (temp_id=%d, hum_id=%d)", 
                 temp_id, hum_id);
    } else {
        ESP_LOGW(TAG, "Failed to publish sensor data (MQTT not connected?)");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Husvert Phase 1.4 - Sensor Integration");
    ESP_LOGI(TAG, "Event-Driven Architecture");
    ESP_LOGI(TAG, "========================================");
    
    // Initialize LED
    ESP_LOGI(TAG, "Step 1: Initializing LED...");
    configure_led();
    set_led_color(0, 0, 10);  // Blue = starting up
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Initialize Sensor with Callback
    ESP_LOGI(TAG, "Step 2: Initializing SHTC3 sensor...");
    // Pass our event handler - sensor will call it when data is ready
    esp_err_t sensor_ret = sensor_init(handle_sensor_data);
    if (sensor_ret != ESP_OK) {
        ESP_LOGW(TAG, "⚠ Sensor init failed — check I2C wiring (SDA=GPIO8, SCL=GPIO9)");
        ESP_LOGW(TAG, "Continuing without sensor");
    } else {
        ESP_LOGI(TAG, "✓ SHTC3 sensor ready (callback registered)");
    }

    // Connect to WiFi
    ESP_LOGI(TAG, "Step 3: Connecting to WiFi...");
    esp_err_t ret = wifi_init_sta();
    
    if (ret != ESP_OK) {
        // WiFi failed - critical error
        ESP_LOGE(TAG, "✗ WiFi connection FAILED!");
        set_led_color(10, 0, 0);  // Red = failed
        ESP_LOGE(TAG, "Cannot continue without WiFi. System halted.");
        return;  // Stop here
    }
    
    // WiFi connected successfully
    ESP_LOGI(TAG, "✓ WiFi connected successfully!");
    set_led_color(0, 10, 0);  // Green = WiFi OK
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Start MQTT Client
    ESP_LOGI(TAG, "Step 4: Starting MQTT client...");
    ret = mqtt_app_start();
    
    if (ret != ESP_OK) {
        // MQTT failed - non-critical (can continue without it)
        ESP_LOGW(TAG, "⚠ MQTT failed to start!");
        set_led_color(10, 10, 0);  // Yellow = degraded mode
        ESP_LOGW(TAG, "Continuing in degraded mode (no MQTT)");
        // Continue anyway - device still works locally
    } else {
        ESP_LOGI(TAG, "✓ MQTT client started (connecting in background...)");
    }
    
    // Register LED Callback
    ESP_LOGI(TAG, "Step 5: Registering LED command callback...");
    mqtt_set_led_callback(handle_led_command);
    ESP_LOGI(TAG, "✓ LED callback registered");
    
    // ===== SYSTEM READY =====
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System Ready!");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MQTT Topics:");
    ESP_LOGI(TAG, "  Subscribe: %s", MQTT_TOPIC_LED_CMD);
    ESP_LOGI(TAG, "  Publish:   %s", MQTT_TOPIC_STATUS);
    ESP_LOGI(TAG, "  Publish:   %s", MQTT_TOPIC_RSSI);
    ESP_LOGI(TAG, "  Publish:   %s", MQTT_TOPIC_TEMPERATURE);
    ESP_LOGI(TAG, "  Publish:   %s", MQTT_TOPIC_HUMIDITY);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "LED Commands (via MQTT):");
    ESP_LOGI(TAG, "  red, green, blue, yellow, purple, cyan, white, off");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering main loop...");
    
    // ===== MAIN LOOP =====
    // Simple heartbeat - just keep system alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        
        // Optional: Log status periodically
        if (mqtt_is_connected()) {
            ESP_LOGD(TAG, "Status: WiFi=✓ MQTT=✓ Sensor=%s", 
                     (sensor_ret == ESP_OK) ? "✓" : "✗");
        } else if (wifi_is_connected()) {
            ESP_LOGD(TAG, "Status: WiFi=✓ MQTT=✗ Sensor=%s", 
                     (sensor_ret == ESP_OK) ? "✓" : "✗");
        } else {
            ESP_LOGD(TAG, "Status: WiFi=✗ MQTT=✗ Sensor=%s", 
                     (sensor_ret == ESP_OK) ? "✓" : "✗");
        }
    }
}