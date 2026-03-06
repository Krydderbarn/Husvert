# ESP32-S3 Blink - LED Control Example

Part of the **Husvert** project - a distributed home monitoring and control system.

## Overview

This project demonstrates basic ESP32-S3 peripheral control using an addressable RGB LED (WS2812/SK6812). It serves as the foundation for Husvert's hardware abstraction layer and provides a template for future component development.

### What it does

- Controls an on-board addressable RGB LED via GPIO48
- Uses the ESP-IDF RMT peripheral for precise timing
- Demonstrates FreeRTOS task delays and logging
- Shows proper component dependency management

## Hardware

- **Board:** ESP32-S3 DevKit (with built-in RGB LED)
- **LED:** SK68XXMINI-HS addressable RGB LED
- **Connection:** GPIO48 → RGB LED data line

### Schematic Reference

```
GPIO48 ──[R17]── RGB_CTRL ──> SK68XX LED
                               │
                        VCC_3V3 ──> VDD
                           GND  ──> VSS
```

## Prerequisites

### Software Requirements

- **ESP-IDF v5.5.0** or later
- **Python 3.8+** (included with ESP-IDF)
- **Git** for version control
- **VS Code** (recommended) with C/C++ extension

### Installation

1. **Install ESP-IDF:**
   - Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
   - Recommended installation path: `C:\esp\esp-idf` (Windows)

2. **Set up the environment:**
   ```powershell
   # Windows PowerShell
   cd C:\esp\esp-idf
   .\export.ps1
   ```

   ```bash
   # Linux/macOS
   cd ~/esp/esp-idf
   . ./export.sh
   ```

## Building the Project

### 1. Clone the repository

```bash
git clone <husvert-repo-url>
cd husvert/src/esp32/blink
```

### 2. Configure the project

The project uses `sdkconfig.defaults` for reproducible builds. On first build, ESP-IDF will generate the full `sdkconfig` file automatically.

To modify configuration:
```bash
idf.py menuconfig
```

After making changes, save them to defaults:
```bash
idf.py save-defconfig
```

### 3. Build

```bash
idf.py build
```

### 4. Flash to device

Connect your ESP32-S3 via the **COM** USB port (not the USB port).

```bash
# Auto-detect port
idf.py flash

# Or specify port explicitly
idf.py -p COM3 flash  # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux
```

### 5. Monitor output

```bash
idf.py monitor
```

Or combine flash and monitor:
```bash
idf.py flash monitor
```

Press `Ctrl+]` to exit the monitor.

## Project Structure

```
blink/
├── main/
│   ├── main.c              # Main application code
│   ├── CMakeLists.txt      # Component build configuration
│   └── idf_component.yml   # Component dependencies
├── CMakeLists.txt          # Project-level build configuration
├── sdkconfig.defaults      # Default configuration (version controlled)
├── .gitignore              # Git ignore rules
└── README.md               # This file
```

## Code Explanation

### Key Components

1. **LED Strip Driver (`led_strip`):**
   - Manages addressable RGB LEDs
   - Uses RMT peripheral for precise WS2812 timing
   - Added via ESP Component Registry

2. **RMT Peripheral:**
   - Hardware-based pulse generator
   - 10MHz resolution for accurate timing
   - No CPU overhead during transmission

3. **FreeRTOS:**
   - `vTaskDelay()` for cooperative multitasking
   - `ESP_LOGI()` for structured logging

### Main Loop Flow

```c
1. Initialize LED strip with RMT backend
2. Loop forever:
   - Set LED to red (R=10, G=0, B=0)
   - Refresh LED (send data via RMT)
   - Delay 1 second
   - Turn LED off
   - Delay 1 second
```

## Customization

### Change LED Color

Edit `main/main.c`:

```c
// Red
led_strip_set_pixel(led_strip, 0, 10, 0, 0);

// Green
led_strip_set_pixel(led_strip, 0, 0, 10, 0);

// Blue
led_strip_set_pixel(led_strip, 0, 0, 0, 10);

// White
led_strip_set_pixel(led_strip, 0, 10, 10, 10);

// Purple
led_strip_set_pixel(led_strip, 0, 10, 0, 10);
```

### Change Blink Speed

```c
// Fast blink (200ms)
vTaskDelay(200 / portTICK_PERIOD_MS);

// Slow blink (2 seconds)
vTaskDelay(2000 / portTICK_PERIOD_MS);
```

### Adjust Brightness

```c
// Dim
uint8_t brightness = 5;

// Medium
uint8_t brightness = 50;

// Bright
uint8_t brightness = 100;

// Maximum (use with caution!)
uint8_t brightness = 255;
```

## Troubleshooting

### LED doesn't blink

1. **Check GPIO number:**
   - Verify `LED_GPIO 48` matches your board
   - Common alternatives: GPIO 38, GPIO 47, GPIO 21

2. **Check LED type:**
   - This code is for addressable RGB LEDs (WS2812/SK6812)
   - Simple LEDs need different code (use `gpio_set_level()`)

3. **Check wiring:**
   - Ensure you're using the **COM** USB port, not **USB** port
   - Verify power LED is on

### Build errors

1. **Component not found:**
   ```bash
   idf.py add-dependency "espressif/led_strip^2.5.5"
   ```

2. **Wrong target:**
   ```bash
   idf.py set-target esp32s3
   idf.py fullclean
   idf.py build
   ```

3. **Clean build:**
   ```bash
   idf.py fullclean
   idf.py build
   ```

### Flash errors

1. **Port permission (Linux):**
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```

2. **Driver issues (Windows):**
   - Install CH340/CP210x drivers
   - Check Device Manager for COM port

3. **Wrong port:**
   ```bash
   # List available ports
   idf.py -p list
   ```

## Development Tools

### VS Code IntelliSense

The project includes `.vscode/c_cpp_properties.json` for IntelliSense support.

**Compiler path:** Adjust if your ESP-IDF toolchain is in a different location:
```json
"compilerPath": "C:/Users/<username>/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32s3-elf-gcc.exe"
```

After building, reload VS Code window:
- `Ctrl+Shift+P` → "Developer: Reload Window"

### Useful Commands

```bash
# Full rebuild
idf.py fullclean build

# Build, flash, and monitor in one command
idf.py build flash monitor

# Just monitor (if already flashed)
idf.py monitor

# Erase flash completely
idf.py erase-flash

# Show build size analysis
idf.py size

# Add component dependency
idf.py add-dependency "namespace/component^version"
```

## Next Steps for Husvert

This blink example establishes the baseline for Husvert development:

- ✅ **Phase 1.1:** Basic ESP-IDF project structure
- ⏭️ **Phase 1.2:** WiFi connectivity
- ⏭️ **Phase 1.3:** MQTT communication
- ⏭️ **Phase 1.4:** Sensor integration (DHT22, motion sensors)
- ⏭️ **Phase 2.x:** Multi-device coordination

## References

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [LED Strip Component Documentation](https://components.espressif.com/components/espressif/led_strip)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
