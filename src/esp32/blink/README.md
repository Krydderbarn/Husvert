# Husvert - ESP32 Home Monitoring

**Author:** Amar  
**Project Type:** Personal Learning Project  
**Current Status:** Phase 1.4 - Sensor Integration ✅

---

## Overview

Learning ESP32 embedded development through building a home monitoring system. Built with ESP-IDF on ESP32-S3.

**Current Features:**
- RGB LED control (visual status indicators)
- WiFi connectivity with automatic retry
- MQTT publish/subscribe messaging
- Remote LED control via MQTT
- Temperature & humidity monitoring (SHTC3)
- Event-driven architecture with callbacks

---

## Hardware

- **Board:** ESP32-S3 DevKit (YD-ESP32-S3)
- **LED:** Onboard RGB LED (GPIO 48)
- **Sensor:** Adafruit SHTC3 (I²C, address 0x70)
  - SDA → GPIO 8
  - SCL → GPIO 9
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

Exit monitor: `Ctrl+T + Ctrl+X`

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
- `husvert/device1/temperature` - Temperature (°C) every 30s
- `husvert/device1/humidity` - Humidity (%RH) every 30s

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
3. Subscribe to `husvert/device1/#` to see all topics
4. Publish to `husvert/device1/led/command` with payload `red`
5. Watch LED change color and sensor data update!

### Using Command Line
```bash
# Publish LED command
mosquitto_pub -h test.mosquitto.org -t "husvert/device1/led/command" -m "red"

# Subscribe to all device topics
mosquitto_sub -h test.mosquitto.org -t "husvert/device1/#" -v
```

**Expected Output:**
```
husvert/device1/status online
husvert/device1/temperature 23.28
husvert/device1/humidity 39.16
husvert/device1/rssi -45
```

---

## Project Structure

```
main/
├── main.c              # Application entry & orchestration
├── wifi_app.c/h        # WiFi connectivity module
├── wifi_config.h       # WiFi credentials (gitignored)
├── mqtt_app.c/h        # MQTT client module
├── mqtt_config.h       # MQTT broker & topic configuration
├── sensor_app.c/h      # SHTC3 sensor module (I²C)
└── CMakeLists.txt      # Build configuration
```

---

## Architecture

```
Application Layer (main.c)
    ↓
Event-Driven Callbacks
    ├─→ Sensor Data → MQTT Publish
    └─→ LED Commands → LED Control
    ↓
Module Layer
    ├── sensor_app.c - I²C sensor reading
    ├── mqtt_app.c   - Pub/sub messaging
    └── wifi_app.c   - Network connectivity
    ↓
ESP-IDF Components (esp_wifi, mqtt, driver)
    ↓
Hardware (ESP32-S3 + SHTC3)
```

**Key Patterns:**
- Event-driven programming (callbacks)
- Loose coupling (modules independent)
- Asynchronous operations (non-blocking)
- Function pointers (sensor → main → mqtt)

### Event-Driven Flow

```c
// sensor_app.c fires event
data_callback(temp, hum);
    ↓
// main.c handles event
handle_sensor_data(temp, hum)
    ↓
// main.c routes to MQTT
mqtt_publish(TOPIC_TEMP, temp_str, 0, 1);
```

**Benefits:**
- Sensor doesn't know about MQTT
- Easy to add new data consumers (logging, display)
- Testable with mock callbacks

---

## Sensor Integration Details

### SHTC3 Wiring

**IMPORTANT:** Connect to **Vin**, NOT 3Vo!

```
SHTC3 Board → ESP32-S3
─────────────────────────
VIN (red)   → 3V3
GND (black) → GND
SDA (blue)  → GPIO 8
SCL (yellow)→ GPIO 9
3Vo         → (leave disconnected)
```

### I²C Communication

- **Address:** 0x70 (fixed)
- **Speed:** 100kHz (reliable)
- **Protocol:** Wake → Measure → Read → Sleep
- **Reading Time:** ~25ms per measurement

### Expected Serial Output

```
I (349) sensor_app: SHTC3 detected at 0x70
I (349) sensor_app: Initial reading: 23.15 °C, 39.23 %
I (15622) sensor_app: Reading #1: 23.11 °C, 39.01 % (success: 100.0%)
I (15632) main: Sensor event received: 23.11 °C, 39.01 %
I (15642) main: Publishing husvert/device1/temperature: 23.11 °C
I (15652) main: Publishing husvert/device1/humidity: 39.01 %
```

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

**Sensor not detected:**
- Check wiring (VIN → 3.3V, NOT 3Vo!)
- Verify I²C pins: SDA=GPIO8, SCL=GPIO9
- Look for "SHTC3 detected at 0x70" in logs
- Try re-seating jumper wires

**I²C NACK errors:**
- Check all 4 wires are connected firmly
- Verify sensor is powered (3.2-3.4V at VIN)
- Sensor may need power cycle (unplug/replug USB)

**Build errors:**
- Missing `wifi_config.h`? Copy the template
- Component not found? Check `main/CMakeLists.txt`
- I²C driver error? Add `driver` to REQUIRES in CMakeLists.txt

---

## Development Roadmap

- ✅ Phase 1.1: LED Control (RMT, WS2812)
- ✅ Phase 1.2: WiFi Connectivity (Station mode, events)
- ✅ Phase 1.3: MQTT Integration (Pub/Sub, callbacks)
- ✅ Phase 1.4: Sensor Integration (SHTC3, I²C, event-driven)
- 📋 Phase 2.1: Local Storage (SD card logging)
- 📋 Phase 2.2: Display (OLED, real-time data)
- 📋 Phase 2.3: Additional Sensors (motion, light)
- 📋 Phase 3: Multi-Device Network
- 📋 Phase 4: Security (TLS, Authentication)

---

## Resources

- [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [WiFi API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- [MQTT API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/mqtt.html)
- [I²C Driver API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c.html)
- [SHTC3 Datasheet](https://www.sensirion.com/resource/datasheet/shtc3)
- ESP-IDF examples: `esp-idf/examples/`
