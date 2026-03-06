#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_err.h"

/**
 * @brief Initialize and connect to WiFi
 * 
 * This function:
 * 1. Initializes NVS (required for WiFi)
 * 2. Creates the default event loop
 * 3. Initializes the TCP/IP stack
 * 4. Creates the WiFi station interface
 * 5. Starts WiFi and attempts connection
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_is_connected(void);

#endif // WIFI_APP_H