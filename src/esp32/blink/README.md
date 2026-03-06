# Husvert - ESP32 Home Monitoring

**Current Status:** Phase 1.2 - WiFi Connectivity ✅

---

## Overview

Learning ESP32 development through a home monitoring system. Built with ESP-IDF on ESP32-S3.

**Current Features:**
- RGB LED control (visual status indicators)
- WiFi connectivity with automatic retry
- Event-driven architecture

---

## Hardware

- **Board:** ESP32-S3 DevKit
- **LED:** Onboard RGB LED (GPIO 48)
- **WiFi:** 2.4 GHz only

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

## LED Status

| Color | Meaning |
|-------|---------|
| 🔵 Blue | Starting up |
| 🟢 Green | Connected |
| 🔴 Red | Failed to connect |

---

## Project Structure

```
main/
├── main.c              # Application entry
├── wifi_app.c/h        # WiFi module
├── wifi_config.h       # Your credentials (gitignored)
└── CMakeLists.txt      # Build config
```

---

## Troubleshooting

**Can't connect to WiFi:**
- Check credentials in `wifi_config.h`
- ESP32 only works with 2.4 GHz networks
- Check signal strength

**Build errors:**
- Missing `wifi_config.h`? Copy the template
- Component not found? Run `idf.py add-dependency "espressif/led_strip"`

---

## Roadmap

- ✅ Phase 1.1: LED Control
- ✅ Phase 1.2: WiFi Connectivity
- 🔄 Phase 1.3: MQTT Client (next)
- 📋 Phase 1.4: Sensor Integration

---

## Resources

- [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [WiFi API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- ESP-IDF examples: `esp-idf/examples/wifi/getting_started/station/`