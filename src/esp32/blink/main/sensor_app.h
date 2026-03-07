/*
 * Sensor Application Module
 * Handles SHTC3 temperature & humidity sensor
 * 
 * Architecture: Event-driven with callbacks
 * - Sensor reads data independently
 * - Fires callback when new data available
 * - Does NOT know about MQTT or other consumers
 */

#ifndef SENSOR_APP_H
#define SENSOR_APP_H

#include "esp_err.h"

/**
 * @brief Sensor data callback function type
 * 
 * Called whenever new sensor data is available.
 * This allows the application to decide what to do with the data
 * (publish to MQTT, log to SD card, display on screen, etc.)
 * 
 * @param temperature Temperature in °C
 * @param humidity Relative humidity in %
 */
typedef void (*sensor_data_callback_t)(float temperature, float humidity);

/**
 * @brief Initialize SHTC3 sensor and start periodic reading task
 * 
 * This function:
 * - Initializes I²C bus
 * - Verifies SHTC3 is present
 * - Starts background task that reads sensor every 30 seconds
 * - Calls the provided callback with new data
 * 
 * @param callback Function to call when new sensor data is available (can be NULL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_init(sensor_data_callback_t callback);

/**
 * @brief Get last temperature reading
 * 
 * @param temperature Output: temperature in °C
 * @return ESP_OK if valid data available, ESP_FAIL otherwise
 */
esp_err_t sensor_get_temperature(float *temperature);

/**
 * @brief Get last humidity reading
 * 
 * @param humidity Output: relative humidity in %
 * @return ESP_OK if valid data available, ESP_FAIL otherwise
 */
esp_err_t sensor_get_humidity(float *humidity);

#endif // SENSOR_APP_H