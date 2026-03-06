#include "wifi_app.h"
#include "wifi_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_app";

// Event group for WiFi status
static EventGroupHandle_t s_wifi_event_group;

// Track connection attempts
static int s_retry_num = 0;

// Connection state
static bool s_is_connected = false;

/**
 * @brief Event handler for WiFi and IP events
 * 
 * This function is called automatically by ESP-IDF when WiFi events occur.
 * It's registered with the event loop in wifi_init_sta().
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // WiFi driver started, now try to connect
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started, attempting connection...");
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // We got disconnected
        s_is_connected = false;
        
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connection to AP, attempt %d/%d", 
                     s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            // Max retries reached
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to AP after %d attempts", WIFI_MAXIMUM_RETRY);
        }
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Successfully got an IP address!
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        
        s_retry_num = 0;  // Reset retry counter
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}