/*
 * MQTT Application Module
 * Handles MQTT broker connection, publishing, and subscribing
 */

#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize and connect to MQTT broker
 * 
 * This function:
 * - Creates MQTT client
 * - Connects to broker
 * - Subscribes to command topics
 * - Starts background publishing task
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_app_start(void);

/**
 * @brief Stop MQTT client and cleanup
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_app_stop(void);

/**
 * @brief Check if MQTT client is connected to broker
 * 
 * @return true if connected, false otherwise
 */
bool mqtt_is_connected(void);

/**
 * @brief Publish a message to a topic
 * 
 * @param topic Topic to publish to
 * @param data Message payload
 * @param len Length of payload (or 0 for null-terminated string)
 * @param qos Quality of Service (0, 1, or 2)
 * @return Message ID on success, -1 on failure
 */
int mqtt_publish(const char *topic, const char *data, int len, int qos);

/**
 * @brief Set LED command callback
 * 
 * Called when LED command is received from MQTT broker
 * 
 * @param callback Function to call with LED command ("red", "green", "blue", "off")
 */
void mqtt_set_led_callback(void (*callback)(const char *command));

#endif // MQTT_APP_H