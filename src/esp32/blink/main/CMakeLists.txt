# Component CMakeLists.txt for main component
# 
# This file tells the build system which source files to compile
# and which components this component depends on.

idf_component_register(
    SRCS 
        "main.c"           # Main application entry point
        "wifi_app.c"       # WiFi connectivity module
        "mqtt_app.c"       # MQTT client module
        "sensor_app.c"     # Sensor reading module (SHTC3)
    
    INCLUDE_DIRS 
        "."                # Current directory (for header files)
    
    REQUIRES 
        led_strip          # RGB LED control component
        esp_wifi           # WiFi driver component
        nvs_flash          # Non-volatile storage (required by WiFi)
        esp_driver_gpio    # GPIO driver (for LED and hardware control)
        mqtt               # MQTT client (built-in ESP-IDF component)
        driver             # For I²C driver (sensor communication)
)