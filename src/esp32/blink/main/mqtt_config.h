/*
 * MQTT Configuration
 * Settings for MQTT broker connection
 * 
 * All MQTT topics are centralized here for easy management
 */

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT Broker Configuration
#define MQTT_BROKER_URI     "mqtt://test.mosquitto.org:1883"

// MQTT Topics - All topics defined in one place
#define MQTT_TOPIC_STATUS       "husvert/device1/status"
#define MQTT_TOPIC_RSSI         "husvert/device1/rssi"
#define MQTT_TOPIC_LED_CMD      "husvert/device1/led/command"
#define MQTT_TOPIC_TEMPERATURE  "husvert/device1/temperature"
#define MQTT_TOPIC_HUMIDITY     "husvert/device1/humidity"

// MQTT Client Settings
#define MQTT_CLIENT_ID      "husvert_esp32_device1"

// Publishing intervals (milliseconds)
#define MQTT_STATUS_INTERVAL    10000   // 10 seconds
#define MQTT_RSSI_INTERVAL      30000   // 30 seconds

#endif // MQTT_CONFIG_H