/*
 * WiFi Application Module
 * Handles WiFi connection in station mode
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "wifi_app.h"
#include "wifi_config.h"

// Logging tag
static const char *TAG = "wifi_app";

// FreeRTOS event group for WiFi synchronization
static EventGroupHandle_t s_wifi_event_group;

// Connection state tracking
static int s_retry_num = 0;
static bool s_is_connected = false;

/**
 * @brief Event handler for WiFi and IP events
 * 
 * This function is called by the ESP event loop when WiFi events occur.
 * It handles three main events:
 * 1. WIFI_EVENT_STA_START - WiFi initialized, start connection
 * 2. WIFI_EVENT_STA_DISCONNECTED - Lost connection, retry
 * 3. IP_EVENT_STA_GOT_IP - Successfully connected and got IP
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // WiFi driver started, initiate connection
        ESP_LOGI(TAG, "WiFi started, connecting...");
        esp_wifi_connect();
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Lost connection or connection failed
        s_is_connected = false;
        
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying connection... (attempt %d/%d)", 
                     s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            // Max retries reached, signal failure
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect after %d attempts", WIFI_MAXIMUM_RETRY);
        }
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Successfully connected and received IP address
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        
        s_retry_num = 0;  // Reset retry counter
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Initialize WiFi in station mode and connect to AP
 * 
 * This function performs the complete WiFi initialization sequence:
 * 1. Initialize NVS (required for WiFi)
 * 2. Create event synchronization
 * 3. Initialize TCP/IP stack
 * 4. Configure WiFi driver
 * 5. Register event handlers
 * 6. Start WiFi and wait for connection
 * 
 * @return ESP_OK on successful connection, ESP_FAIL otherwise
 */
esp_err_t wifi_init_sta(void)
{
    // Step 1: Initialize NVS (required by WiFi for calibration data)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated, erase and retry
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Step 2: Create event group for synchronization
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    // Step 3: Initialize TCP/IP stack (required before WiFi)
    ESP_ERROR_CHECK(esp_netif_init());

    // Step 4: Create default event loop (handles WiFi and IP events)
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Step 5: Create default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Step 6: Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Step 7: Register event handlers
    // Handle WiFi events (start, disconnect)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                          ESP_EVENT_ANY_ID,
                                                          &event_handler,
                                                          NULL,
                                                          NULL));
    
    // Handle IP events (got IP)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                          IP_EVENT_STA_GOT_IP,
                                                          &event_handler,
                                                          NULL,
                                                          NULL));

    // Step 8: Configure WiFi credentials and settings
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            // Optional: improve connection reliability
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Set WiFi mode to station (client)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Apply WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Step 9: Start WiFi (triggers WIFI_EVENT_STA_START)
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete. Waiting for connection...");

    // Step 10: Wait for connection result
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,  // Don't clear bits
                                            pdFALSE,  // Wait for any bit
                                            portMAX_DELAY);  // Wait forever

    // Step 11: Check result and return appropriate status
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: %s", WIFI_SSID);
        s_is_connected = true;
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
        s_is_connected = false;
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Unexpected event");
        return ESP_FAIL;
    }
}

/**
 * @brief Check if WiFi is currently connected
 * 
 * @return true if connected to AP and have IP address, false otherwise
 */
bool wifi_is_connected(void)
{
    return s_is_connected;
}