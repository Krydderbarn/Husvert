# Husvert - ESP32 Home Monitoring

**Author:** Amar  
**Project Type:** Personal Learning Project  
**Current Status:** Phase 1.3 - MQTT Integration ✅

---

## Overview

Learning ESP32 embedded development through building a home monitoring system. Built with ESP-IDF on ESP32-S3.

**Current Features:**
- RGB LED control (visual status indicators)
- WiFi connectivity with automatic retry
- MQTT publish/subscribe messaging
- Remote LED control via MQTT
- Event-driven architecture

---

## Hardware

- **Board:** ESP32-S3 DevKit
- **LED:** Onboard RGB LED (GPIO 48)
- **WiFi:** 2.4 GHz only
- **MQTT Broker:** test.mosquitto.org (public test broker)

---

## Quick Start

### Setup

1. Install [ESP-IDF v5.5.0](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Load environment: `.\export.ps1` (Windows) or `. ./export.sh` (Linux/Mac)
3. Set target: `idf.py set-target esp32s3`

### Configure WiFi

```bash
cp main/wifi_config.h.template main/wifi_config.h
# Edit wifi_config.h with your network credentials
```

### Build and Flash

```bash
idf.py build flash monitor
```

Exit monitor: `Ctrl+]`

---

## Features

### LED Status Indicators

| Color | Meaning |
|-------|---------|
| 🔵 Blue | Starting up |
| 🟢 Green | WiFi connected, system ready |
| 🔴 Red | WiFi failed (critical) |
| 🟡 Yellow | MQTT failed (degraded mode) |

### MQTT Topics

**Published by ESP32:**
- `husvert/device1/status` - Device status ("online") every 10s
- `husvert/device1/rssi` - WiFi signal strength (dBm) every 30s

**Subscribed by ESP32:**
- `husvert/device1/led/command` - LED control commands

### LED Commands

Send these via MQTT to control the LED:
- `red`, `green`, `blue`, `yellow`, `purple`, `cyan`, `white`, `off`

---

## Testing MQTT

### Using MQTT Explorer (GUI)
1. Download: http://mqtt-explorer.com/
2. Connect to `test.mosquitto.org`
3. Publish to `husvert/device1/led/command` with payload `red`
4. Watch LED change color!

### Using Command Line
```bash
# Publish LED command
mosquitto_pub -h test.mosquitto.org -t "husvert/device1/led/command" -m "red"

# Subscribe to all device topics
mosquitto_sub -h test.mosquitto.org -t "husvert/device1/#" -v
```

---

## Project Structure

```
main/
├── main.c              # Application entry point
├── wifi_app.c/h        # WiFi connectivity module
├── wifi_config.h       # WiFi credentials (gitignored)
├── mqtt_app.c/h        # MQTT client module
├── mqtt_config.h       # MQTT broker settings
└── CMakeLists.txt      # Build configuration
```

---

## Architecture

```
Application Layer (main.c)
    ↓
MQTT Layer (mqtt_app.c) - Pub/sub messaging
    ↓
WiFi Layer (wifi_app.c) - Network connectivity
    ↓
ESP-IDF Components (esp_wifi, mqtt, nvs_flash)
    ↓
Hardware (ESP32-S3)
```

**Key Patterns:**
- Event-driven programming (callbacks)
- Modular design (separate WiFi/MQTT modules)
- Asynchronous operations (non-blocking)
- Function pointers (loose coupling)

---

## Troubleshooting

**Can't connect to WiFi:**
- Check credentials in `wifi_config.h`
- ESP32 only works with 2.4 GHz networks
- Check signal strength

**MQTT not connecting:**
- Verify WiFi is connected (green LED)
- Check serial monitor for connection errors
- Public broker may be overloaded (try again later)

**Build errors:**
- Missing `wifi_config.h`? Copy the template
- Component not found? Check `main/CMakeLists.txt`

---

## Development Roadmap

- ✅ Phase 1.1: LED Control
- ✅ Phase 1.2: WiFi Connectivity  
- ✅ Phase 1.3: MQTT Integration
- 📋 Phase 1.4: Sensor Integration (DHT22, PIR)
- 📋 Phase 2: Multi-Device Coordination
- 📋 Phase 3: OTA Updates, Web Interface
- 📋 Phase 4: Security (TLS, Authentication)

---

## Resources

- [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [WiFi API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- [MQTT API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/mqtt.html)
- ESP-IDF examples: `esp-idf/examples/`

---

## License

Personal learning project - feel free to learn from it!