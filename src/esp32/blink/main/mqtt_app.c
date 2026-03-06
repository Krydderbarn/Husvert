/*
 * MQTT Application Module
 * Handles MQTT broker connection, publishing, and subscribing
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"
#include "mqtt_config.h"
#include "esp_wifi.h"

// Logging tag
static const char *TAG = "mqtt_app";

// MQTT client handle
static esp_mqtt_client_handle_t mqtt_client = NULL;

// Connection state
static bool is_connected = false;

// LED command callback
static void (*led_command_callback)(const char *command) = NULL;

// Forward declarations
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);
static void mqtt_publishing_task(void *pvParameters);

/**
 * @brief MQTT event handler
 * 
 * This function is called when MQTT events occur (connected, disconnected, etc.)
 * It's the heart of the MQTT client - handles all asynchronous events.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        
        case MQTT_EVENT_CONNECTED:
            // Successfully connected to broker
            ESP_LOGI(TAG, "Connected to MQTT broker");
            is_connected = true;
            
            // Subscribe to LED command topic
            int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_LED_CMD, 1);
            ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", MQTT_TOPIC_LED_CMD, msg_id);
            
            // Publish initial status
            esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS, "online", 0, 1, 0);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            // Lost connection to broker
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            is_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            // Successfully subscribed to a topic
            ESP_LOGI(TAG, "Subscription successful, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            // Message successfully published
            ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            // Received data on subscribed topic
            ESP_LOGI(TAG, "Received message:");
            ESP_LOGI(TAG, "  Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "  Data: %.*s", event->data_len, event->data);
            
            // Check if it's an LED command
            if (strncmp(event->topic, MQTT_TOPIC_LED_CMD, event->topic_len) == 0) {
                // Extract command from payload
                char command[32] = {0};
                int len = (event->data_len < 31) ? event->data_len : 31;
                strncpy(command, event->data, len);
                
                ESP_LOGI(TAG, "LED Command received: %s", command);
                
                // Call callback if registered
                if (led_command_callback != NULL) {
                    led_command_callback(command);
                } else {
                    ESP_LOGW(TAG, "No LED callback registered!");
                }
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error occurred");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP transport error");
            }
            break;
            
        default:
            ESP_LOGD(TAG, "Other MQTT event id: %d", event->event_id);
            break;
    }
}

/**
 * @brief Initialize and start MQTT client
 * 
 * Creates MQTT client, configures it, and connects to broker.
 */
esp_err_t mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Starting MQTT client...");
    
    // MQTT client configuration
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
    };
    
    // Create MQTT client
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT client");
        return ESP_FAIL;
    }
    
    // Register event handler
    esp_err_t err = esp_mqtt_client_register_event(mqtt_client, 
                                                    ESP_EVENT_ANY_ID,
                                                    mqtt_event_handler, 
                                                    NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");
        return err;
    }
    
    // Start MQTT client (begins connection to broker)
    err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return err;
    }
    
    ESP_LOGI(TAG, "MQTT client started, connecting to broker...");
    
    // Create publishing task (runs in background)
    BaseType_t ret = xTaskCreate(mqtt_publishing_task, 
                                  "mqtt_pub_task", 
                                  4096, 
                                  NULL, 
                                  5, 
                                  NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create publishing task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Publishing task - runs in background
 * 
 * Periodically publishes device status and WiFi signal strength
 */
static void mqtt_publishing_task(void *pvParameters)
{
    uint32_t status_counter = 0;
    uint32_t rssi_counter = 0;
    
    ESP_LOGI(TAG, "Publishing task started");
    
    while (1) {
        // Wait 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Only publish if connected
        if (!is_connected) {
            continue;
        }
        
        status_counter += 1000;
        rssi_counter += 1000;
        
        // Publish status every 10 seconds
        if (status_counter >= MQTT_STATUS_INTERVAL) {
            status_counter = 0;
            
            ESP_LOGI(TAG, "Publishing status...");
            int msg_id = esp_mqtt_client_publish(mqtt_client, 
                                                 MQTT_TOPIC_STATUS, 
                                                 "online", 
                                                 0, 1, 0);
            if (msg_id < 0) {
                ESP_LOGE(TAG, "Failed to publish status");
            }
        }
        
        // Publish RSSI every 30 seconds
        if (rssi_counter >= MQTT_RSSI_INTERVAL) {
            rssi_counter = 0;
            
            // Get WiFi RSSI (signal strength)
            wifi_ap_record_t ap_info;
            esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
            if (ret == ESP_OK) {
                char rssi_str[16];
                snprintf(rssi_str, sizeof(rssi_str), "%d", ap_info.rssi);
                
                ESP_LOGI(TAG, "Publishing RSSI: %s dBm", rssi_str);
                int msg_id = esp_mqtt_client_publish(mqtt_client, 
                                                     MQTT_TOPIC_RSSI, 
                                                     rssi_str, 
                                                     0, 1, 0);
                if (msg_id < 0) {
                    ESP_LOGE(TAG, "Failed to publish RSSI");
                }
            } else {
                ESP_LOGW(TAG, "Failed to get WiFi RSSI");
            }
        }
    }
}

/**
 * @brief Stop MQTT client
 */
esp_err_t mqtt_app_stop(void)
{
    if (mqtt_client) {
        ESP_LOGI(TAG, "Stopping MQTT client...");
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        is_connected = false;
    }
    return ESP_OK;
}

/**
 * @brief Check if MQTT is connected
 */
bool mqtt_is_connected(void)
{
    return is_connected;
}

/**
 * @brief Publish a message
 */
int mqtt_publish(const char *topic, const char *data, int len, int qos)
{
    if (!is_connected) {
        ESP_LOGW(TAG, "Cannot publish - not connected to broker");
        return -1;
    }
    
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, 0);
}

/**
 * @brief Register LED command callback
 */
void mqtt_set_led_callback(void (*callback)(const char *command))
{
    led_command_callback = callback;
    ESP_LOGI(TAG, "LED callback registered");
}