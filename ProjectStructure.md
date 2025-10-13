# Project Structure Documentation

## Automatic Soldering Station based on ESP32

This document describes the complete project structure, purpose of each directory and file, and the organization principles used in this embedded systems project.

---

## Project Overview

An automated soldering station controller based on ESP32 microcontroller. The system:
- Hosts a WiFi access point and web server
- Accepts G-Code files for automated soldering operations
- Controls 4 stepper motors (X, Y, Z axes + solder supply)
- Manages soldering iron temperature via PWM
- Provides real-time status via web interface
- Optional OLED display for local status monitoring

---

## Directory Structure

```
Automatic-Soldering-Station-based-on-ESP32/
├── components/              # Reusable ESP-IDF components
│   ├── stepper_motor/       # Stepper motor driver (C HAL + C++ API)
│   ├── soldering_iron/      # Soldering iron controller (C HAL + C++ API)
│   ├── temperature_sensor/  # Temperature measurement (C HAL + C++ API)
│   ├── motion_controller/   # Multi-axis motion coordination
│   ├── gcode_parser/        # G-Code parsing and execution
│   ├── wifi_manager/        # WiFi AP management
│   ├── web_server/          # HTTP server for web interface
│   ├── filesystem/          # LittleFS file management
│   └── display/             # Optional OLED display driver
├── main/                    # Main application
│   ├── main.cpp             # Application entry point
│   ├── CMakeLists.txt       # Main component build config
│   └── Kconfig.projbuild    # Project configuration options
├── web_interface/           # Web UI files (HTML/CSS/JS)
│   ├── index.html           # Main web page
│   ├── style.css            # Stylesheet
│   ├── app.js               # Main application logic
│   ├── gcode_validator.js   # Client-side G-Code validation
│   └── visualizer.js        # G-Code path visualization
├── tools/                   # Development tools
│   └── drill_to_gcode.py    # Drill file to G-Code converter
├── docs/                    # Additional documentation
├── CMakeLists.txt           # Root CMake configuration
├── platformio.ini           # PlatformIO project configuration
├── partitions.csv           # Flash partition table
├── sdkconfig.defaults       # Default ESP-IDF configuration
├── README.md                # Project README
└── ProjectStructure.md      # This file

```

---

## Component Details

### 1. Hardware Abstraction Layer Components

All hardware components follow a dual-layer architecture:
- **C HAL Layer**: Low-level hardware access using ESP-IDF APIs
- **C++ API Layer**: Object-oriented wrapper for application-level use

This design provides:
- Portability and testability
- Clear separation of concerns
- Modern C++ features (RAII, move semantics)
- Compatibility with both C and C++ code

#### stepper_motor/

Controls DRV8825 stepper motor drivers for X, Y, Z, and solder supply axes.

**Files:**
- `include/stepper_motor_hal.h` - C HAL interface definition
- `include/StepperMotor.hpp` - C++ wrapper class
- `stepper_motor_hal.c` - HAL implementation
- `StepperMotor.cpp` - C++ wrapper implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- Microstepping support (1/2, 1/4, 1/8, 1/16, 1/32)
- Direction and enable control
- Position tracking
- Configurable GPIO pins

#### soldering_iron/

Manages soldering iron heating element via PWM control.

**Files:**
- `include/soldering_iron_hal.h` - C HAL interface
- `include/SolderingIron.hpp` - C++ wrapper
- `soldering_iron_hal.c` - HAL implementation
- `SolderingIron.cpp` - C++ wrapper implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- PWM-based power control (drives IRLZ44N MOSFET)
- Temperature setpoint management
- PID control loop integration
- Safety limits

#### temperature_sensor/

Reads temperature from AD8495 K-type thermocouple amplifier.

**Files:**
- `include/temperature_sensor_hal.h` - C HAL interface
- `include/TemperatureSensor.hpp` - C++ wrapper
- `temperature_sensor_hal.c` - HAL implementation
- `TemperatureSensor.cpp` - C++ wrapper implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- ADC-based voltage reading
- Temperature calculation from voltage
- Multi-sample averaging for noise reduction
- Calibration support

### 2. Control and Logic Components

#### motion_controller/

Coordinates movement of multiple stepper motors for synchronized motion.

**Files:**
- `include/motion_controller.h` - API definition
- `motion_controller.cpp` - Implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- 3D position tracking (X, Y, Z)
- Coordinated multi-axis movements
- Work area boundary validation
- Emergency stop functionality
- Solder feed control

#### gcode_parser/

Parses and executes G-Code commands.

**Files:**
- `include/gcode_parser.h` - Parser API
- `include/gcode_executor.h` - Executor API
- `gcode_parser.c` - Parser implementation
- `gcode_executor.cpp` - Executor implementation
- `CMakeLists.txt` - Build configuration

**Supported Commands:**
- `G0/G1` - Linear movement to position (X, Y, Z)
- `G28` - Home all axes
- `G4` - Dwell/pause
- `M104` - Set temperature
- Custom solder feed command

**Features:**
- Line-by-line parsing
- Command validation
- Program buffering
- Progress tracking
- Time estimation

### 3. Network and Communication Components

#### wifi_manager/

Manages ESP32 WiFi access point for client connections.

**Files:**
- `include/wifi_manager.h` - API definition
- `wifi_manager.c` - Implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- Configurable SSID and password (via Kconfig)
- Client connection monitoring
- IP address management
- Network event handling

#### web_server/

HTTP server for serving web interface and handling API requests.

**Files:**
- `include/web_server.h` - API definition
- `web_server.cpp` - Implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- Static file serving (HTML, CSS, JS)
- RESTful API endpoints
- WebSocket support for real-time updates
- G-Code upload handling
- Status broadcasting

#### filesystem/

LittleFS filesystem manager for web content storage.

**Files:**
- `include/filesystem.h` - API definition
- `filesystem.c` - Implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- LittleFS partition mounting
- File read/write operations
- Storage usage monitoring
- Web file management

### 4. Display Component (Optional)

#### display/

Optional OLED display driver for local status visualization.

**Files:**
- `include/display.h` - API definition
- `display.c` - Implementation
- `CMakeLists.txt` - Build configuration

**Features:**
- I2C OLED communication
- Status information display:
  - Current position (X, Y, Z)
  - Temperature
  - Time remaining
  - System uptime
- Enable/disable via Kconfig

---

## Main Application

### main/

Application entry point and initialization.

**Files:**
- `main.cpp` - Application entry point with `app_main()`
- `CMakeLists.txt` - Main component dependencies
- `Kconfig.projbuild` - Project-specific configuration options

**Responsibilities:**
- Initialize NVS (Non-Volatile Storage)
- Configure all hardware components
- Start WiFi AP and web server
- Initialize filesystem and load web files
- Start G-Code executor task
- Main event loop

---

## Web Interface

### web_interface/

Client-side web application for user interaction.

**Files:**
- `index.html` - Main page structure
- `style.css` - Visual styling
- `app.js` - Main application logic
- `gcode_validator.js` - G-Code validation
- `visualizer.js` - Path visualization

**Features:**
- G-Code file upload
- Real-time visualization of toolpath
- Manual motion control
- Temperature adjustment
- Start/stop/pause controls
- Live status updates via WebSocket
- Progress monitoring

---

## Tools

### tools/

Development and utility scripts.

**Files:**
- `drill_to_gcode.py` - Python script to convert drill files to G-Code

**Usage:**
```bash
python tools/drill_to_gcode.py input.drl output.gcode
```

---

## Configuration Files

### Root Level Configuration

#### CMakeLists.txt
ESP-IDF project root CMake configuration. Includes ESP-IDF build system and defines project name.

#### platformio.ini
PlatformIO project configuration:
- Platform: `espressif32`
- Board: `esp32doit-devkit-v1`
- Framework: `espidf`

#### partitions.csv
Flash memory partition table:
- `nvs` - Non-volatile storage (16KB)
- `otadata` - OTA data (8KB)
- `phy_init` - PHY initialization data (4KB)
- `factory` - Application binary (1MB)
- `littlefs` - Web files storage (1MB)

#### sdkconfig.defaults
Default ESP-IDF SDK configuration:
- Custom partition table
- FreeRTOS tick rate (1000Hz)
- C++ exceptions enabled
- WiFi buffer configuration
- Network stack settings

#### main/Kconfig.projbuild
Project-specific Kconfig options accessible via `idf.py menuconfig`:

**Configuration Sections:**
1. **WiFi Configuration**
   - SSID, password, channel, max connections

2. **Motor Pin Assignments**
   - X, Y, Z, and solder supply motor pins
   - Step, direction, enable pins for each axis
   - DRV8825 microstepping mode pins

3. **Soldering Iron**
   - PWM pin, frequency
   - Temperature limits and defaults

4. **Temperature Sensor**
   - ADC channel selection
   - Sampling parameters

5. **Motion Control**
   - Maximum velocities per axis
   - Work area dimensions
   - Steps per millimeter calibration

6. **Display (Optional)**
   - Enable/disable OLED
   - I2C pins and address

7. **Web Server**
   - HTTP port
   - Handler limits

---

## Build System

### ESP-IDF Component System

Each component directory contains:
- `CMakeLists.txt` - Component build configuration
- `include/` - Public header files
- Source files (`.c`, `.cpp`)

Components declare dependencies via `REQUIRES` in CMakeLists.txt:

```cmake
idf_component_register(
    SRCS "source1.c" "source2.cpp"
    INCLUDE_DIRS "include"
    REQUIRES component1 component2
)
```

### Build Process

```bash
# Configure project
idf.py menuconfig

# Build project
idf.py build

# Flash to ESP32
idf.py flash

# Monitor serial output
idf.py monitor
```

Or using PlatformIO:
```bash
pio run           # Build
pio run -t upload # Flash
pio device monitor # Monitor
```

---

## Embedded Development Best Practices

This project follows industry best practices for embedded C/C++ development:

### 1. Type Safety
- Use fixed-width integer types: `int16_t`, `int32_t`, `uint8_t`, `uint16_t`, etc.
- Avoid plain `int`, `char` for portable code
- Use `double` instead of `float` for precision

### 2. Code Organization
- Separation of HAL (Hardware Abstraction Layer) and application logic
- Component-based architecture for modularity
- Clear public API via header files

### 3. Documentation
- File-level documentation describing purpose
- Function/method documentation
- Inline comments for complex logic
- This comprehensive structure document

### 4. Memory Management
- RAII in C++ for automatic resource cleanup
- Explicit initialization and deinitialization functions
- Handle-based APIs to hide implementation details

### 5. Hardware Abstraction
- Platform-independent interfaces where possible
- Configuration via Kconfig for easy customization
- Testable components with clear interfaces

---

## Hardware Connections

### ESP32 DOIT DevKit V1

**Stepper Motors (DRV8825 Drivers):**
- X-Axis: GPIO 25 (STEP), 26 (DIR), 27 (EN)
- Y-Axis: GPIO 14 (STEP), 12 (DIR), 13 (EN)
- Z-Axis: GPIO 33 (STEP), 32 (DIR), 15 (EN)
- Solder Supply: GPIO 18 (STEP), 19 (DIR), 21 (EN)
- Microstepping: GPIO 16 (M0), 17 (M1), 5 (M2) - shared by all drivers

**Soldering Iron:**
- PWM Control: GPIO 22 → IRLZ44N MOSFET gate → Heating element

**Temperature Sensor:**
- AD8495 Output → ADC1 Channel 0 (GPIO 36)

**Display (Optional):**
- I2C SDA: GPIO 23
- I2C SCL: GPIO 22

**Power:**
- 24V DC input for heater and motors
- LM2596 DC-DC converter: 24V → 5V for ESP32 VIN

---

## Workflow

### Development Workflow

1. **PCB Design**: Create PCB in EDA software, export drill file
2. **Conversion**: Run `drill_to_gcode.py` to convert drill file
3. **ESP32 Setup**: Flash firmware to ESP32
4. **Connection**: Connect to WiFi hotspot (default: "SolderingStation")
5. **Web Interface**: Open browser to `http://192.168.4.1`
6. **Upload G-Code**: Load converted G-Code file
7. **Visualization**: Review toolpath visualization
8. **Parameter Setup**: Set temperature and other parameters
9. **Execution**: Start soldering process
10. **Monitoring**: Track progress and status in real-time

### System Operation

1. ESP32 boots and initializes all components
2. WiFi AP starts, web server begins serving files
3. User connects and uploads G-Code
4. Client-side validation checks G-Code syntax
5. G-Code sent to ESP32 via HTTP POST
6. Server-side validation (bounds checking)
7. G-Code stored in memory, ready for execution
8. User triggers start
9. G-Code executor processes commands sequentially
10. Motion controller coordinates stepper movements
11. Temperature controller maintains soldering temperature
12. Real-time status updates broadcast via WebSocket
13. Optional OLED shows local status
14. Completion notification sent to web interface

---

## Extension Points

The project structure is designed for extensibility:

### Adding New Hardware
1. Create new component in `components/`
2. Implement C HAL in `.h` and `.c` files
3. Add C++ wrapper if needed (`.hpp`, `.cpp`)
4. Add Kconfig options to `main/Kconfig.projbuild`
5. Update `main/CMakeLists.txt` dependencies

### Adding Web Features
1. Add HTML/CSS/JS to `web_interface/`
2. Register new URI handlers in `web_server` component
3. Add API endpoints for data exchange
4. Update WebSocket message format if needed

### Adding G-Code Commands
1. Extend `gcode_command_type_t` enum in `gcode_parser.h`
2. Update parser logic in `gcode_parser.c`
3. Implement execution in `gcode_executor.cpp`
4. Update web validator in `gcode_validator.js`

---

## Future Enhancements

Potential improvements to the system:
- Camera integration for visual feedback
- Automatic solder detection
- Machine learning for optimal parameters
- Multi-language web interface
- OTA (Over-The-Air) firmware updates
- SD card support for large G-Code files
- Job history and statistics
- User authentication

---

## References

1. ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/
2. DRV8825 Datasheet
3. AD8495 Thermocouple Amplifier Datasheet
4. LittleFS Documentation
5. G-Code Reference

---

## License

This project was developed as part of the "Computer Organization Principles" course at Ukrainian Catholic University (UCU).

---

**Document Version**: 1.0  
**Last Updated**: October 2025  
**Maintainer**: UCU Automatic Soldering Station Team
