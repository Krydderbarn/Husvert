/*
 * Sensor Application Module
 * Handles SHTC3 sensor reading
 * 
 * Architecture: Event-driven - fires callbacks, doesn't know about MQTT
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "sensor_app.h"

static const char *TAG = "sensor_app";

// I²C Configuration
#define I2C_MASTER_SCL_IO    9
#define I2C_MASTER_SDA_IO    8
#define SHTC3_ADDR           0x70

// Sensor reading interval (milliseconds)
#define SENSOR_READ_INTERVAL_MS  30000

// I²C handles
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// Callback for sensor data
static sensor_data_callback_t data_callback = NULL;

// Last sensor readings
static float last_temperature = 0.0;
static float last_humidity = 0.0;
static bool data_valid = false;

// Forward declaration
static void sensor_task(void *pvParameters);

/**
 * @brief Send wake-up command to SHTC3
 */
static esp_err_t shtc3_wakeup(void)
{
    uint8_t cmd[2] = {0x35, 0x17};
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, 2, 1000);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms wake-up time
    }
    return ret;
}

/**
 * @brief Send sleep command to SHTC3
 */
static esp_err_t shtc3_sleep(void)
{
    uint8_t cmd[2] = {0xB0, 0x98};
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, 2, 1000);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return ret;
}

/**
 * @brief Read temperature and humidity from SHTC3
 */
static esp_err_t shtc3_read(float *temperature, float *humidity)
{
    esp_err_t ret;
    
    // Wake up sensor
    ret = shtc3_wakeup();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Send measurement command (0x7CA2 - RH first, normal mode)
    uint8_t cmd[2] = {0x7C, 0xA2};
    ret = i2c_master_transmit(dev_handle, cmd, 2, 1000);
    if (ret != ESP_OK) {
        shtc3_sleep();
        return ret;
    }
    
    // Wait for measurement (25ms)
    vTaskDelay(pdMS_TO_TICKS(25));
    
    // Read 6 bytes
    uint8_t data[6];
    ret = i2c_master_receive(dev_handle, data, 6, 1000);
    if (ret != ESP_OK) {
        shtc3_sleep();
        return ret;
    }
    
    // Parse humidity (first, RH-first command)
    uint16_t raw_hum = (data[0] << 8) | data[1];
    *humidity = 100.0 * ((float)raw_hum / 65536.0);
    
    // Parse temperature (second)
    uint16_t raw_temp = (data[3] << 8) | data[4];
    *temperature = -45.0 + 175.0 * ((float)raw_temp / 65536.0);
    
    // Put sensor back to sleep
    shtc3_sleep();
    
    return ESP_OK;
}

/**
 * @brief Sensor reading task
 * 
 * Runs in background, reads sensor periodically and fires callback
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started (30s interval)");
    
    int read_count = 0;
    int fail_count = 0;
    
    while (1) {
        float temp, hum;
        
        read_count++;
        
        esp_err_t ret = shtc3_read(&temp, &hum);
        
        if (ret == ESP_OK) {
            // Sanity check
            if (temp < -40 || temp > 125 || hum < 0 || hum > 100) {
                ESP_LOGW(TAG, "Sensor values out of range: T=%.2f, RH=%.2f", temp, hum);
                fail_count++;
            } else {
                // Update last readings
                last_temperature = temp;
                last_humidity = hum;
                data_valid = true;
                
                float success_rate = 100.0 * (read_count - fail_count) / read_count;
                ESP_LOGI(TAG, "Reading #%d: %.2f °C, %.2f %% (success: %.1f%%)", 
                         read_count, temp, hum, success_rate);
                
                // Fire callback if registered
                if (data_callback != NULL) {
                    data_callback(temp, hum);
                } else {
                    ESP_LOGD(TAG, "No callback registered - data not consumed");
                }
            }
        } else {
            fail_count++;
            float success_rate = 100.0 * (read_count - fail_count) / read_count;
            ESP_LOGE(TAG, "Failed to read sensor: %s (success: %.1f%%)", 
                     esp_err_to_name(ret), success_rate);
            data_valid = false;
        }
        
        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

/**
 * @brief Initialize sensor
 */
esp_err_t sensor_init(sensor_data_callback_t callback)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing SHTC3 sensor...");
    ESP_LOGI(TAG, "I²C pins: SDA=GPIO%d, SCL=GPIO%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    
    // Store callback
    data_callback = callback;
    if (callback == NULL) {
        ESP_LOGW(TAG, "No callback provided - sensor data will not be consumed");
    }
    
    // Configure I²C bus
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I²C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Add SHTC3 device (50kHz for reliability)
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_ADDR,
        .scl_speed_hz = 50000,  // 50kHz (slower = more reliable)
    };
    
    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I²C device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Verify sensor is present
    ret = i2c_master_probe(bus_handle, SHTC3_ADDR, 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SHTC3 not found at address 0x%02X", SHTC3_ADDR);
        return ret;
    }
    
    ESP_LOGI(TAG, "SHTC3 detected at 0x%02X", SHTC3_ADDR);
    
    // Do initial read to verify it works
    float temp, hum;
    ret = shtc3_read(&temp, &hum);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initial sensor read failed");
        return ret;
    }
    
    ESP_LOGI(TAG, "Initial reading: %.2f °C, %.2f %%", temp, hum);
    
    // Start sensor reading task
    BaseType_t task_ret = xTaskCreate(sensor_task, 
                                       "sensor_task", 
                                       4096, 
                                       NULL, 
                                       5, 
                                       NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sensor initialized successfully!");
    return ESP_OK;
}

/**
 * @brief Get last temperature reading
 */
esp_err_t sensor_get_temperature(float *temperature)
{
    if (!data_valid) {
        return ESP_FAIL;
    }
    *temperature = last_temperature;
    return ESP_OK;
}

/**
 * @brief Get last humidity reading
 */
esp_err_t sensor_get_humidity(float *humidity)
{
    if (!data_valid) {
        return ESP_FAIL;
    }
    *humidity = last_humidity;
    return ESP_OK;
}