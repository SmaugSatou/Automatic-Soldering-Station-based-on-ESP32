# Automatic Soldering Station based on ESP32

An automated soldering station controller for PCB assembly, built with ESP32 microcontroller and ESP-IDF framework.

Authors' semester project, developed as a part of the "Computer Organization Principles" course at Ukrainian Catholic University (UCU).

## Features

- **WiFi Access Point**: Host web interface for wireless control
- **Web-Based Control**: Upload G-Code, visualize toolpaths, monitor status
- **4-Axis Motion Control**: X, Y, Z movement + solder supply
- **Temperature Management**: Closed-loop control of soldering iron
- **Real-Time Monitoring**: WebSocket-based status updates
- **G-Code Support**: Custom dialect for soldering operations
- **Configurable**: All pins and parameters via Kconfig

## Hardware

- **MCU**: ESP32 DOIT DevKit V1
- **Motor Drivers**: 4x DRV8825 (supports 1/32 microstepping)
- **Stepper Motors**: 42SHDC3025-24B (0.9A)
- **Temperature Sensor**: AD8495 K-Type thermocouple amplifier
- **Heater Control**: IRLZ44N MOSFET for PWM control
- **Power**: 24V/3A + LM2596 buck converter (24V→5V)
- **Display (Optional)**: OLED I2C display

## Quick Start

### Prerequisites

- ESP-IDF v4.4 or later
- Python 3.7+ (for drill file conversion)
- Serial drivers for ESP32

### Build and Flash

```bash
# Clone repository
git clone <repository-url>
cd Automatic-Soldering-Station-based-on-ESP32

# Configure project
idf.py menuconfig

# Build
idf.py build

# Flash to ESP32
idf.py flash monitor
```

### Using PlatformIO

```bash
# Build
pio run

# Flash
pio run -t upload

# Monitor
pio device monitor
```

## Usage

1. **Power on ESP32** - WiFi hotspot "SolderingStation" will start
2. **Connect to WiFi** - Password: "soldering123" (configurable)
3. **Open browser** - Navigate to http://192.168.4.1
4. **Upload G-Code** - Load your soldering program
5. **Configure parameters** - Set temperature, speed, etc.
6. **Start job** - Monitor progress in real-time

### Converting Drill Files

Use the provided tool to convert PCB drill files to G-Code:

```bash
python tools/drill_to_gcode.py input.drl output.gcode
```

## Configuration

All hardware pins and parameters are configurable via menuconfig:

```bash
idf.py menuconfig
```

Navigate to: `Automatic Soldering Station Configuration`

Configure:
- WiFi credentials
- Motor pin assignments
- Temperature limits
- Motion parameters
- Display settings

## Project Structure

See [ProjectStructure.md](ProjectStructure.md) for detailed documentation of the project organization.

```
├── components/         # Reusable ESP-IDF components
├── main/              # Main application
├── web_interface/     # HTML/CSS/JS files
├── tools/             # Development tools
└── docs/              # Additional documentation
```

## Components

- **stepper_motor**: DRV8825 driver control (C HAL + C++ API)
- **soldering_iron**: Heater control with PID
- **temperature_sensor**: AD8495 thermocouple interface
- **motion_controller**: Multi-axis coordination
- **gcode_parser**: G-Code parsing and execution
- **wifi_manager**: WiFi AP management
- **web_server**: HTTP server with WebSocket
- **filesystem**: LittleFS for web files
- **display**: Optional OLED status display

## G-Code Commands

Supported commands:
- `G0/G1 X Y Z` - Move to position
- `G28` - Home all axes
- `G4 P` - Dwell/pause
- `M104 S` - Set temperature
- Custom solder feed commands

## Development

### Code Style

- Fixed-width types: `int32_t`, `uint8_t`, etc.
- Use `double` instead of `float`
- Document all public APIs
- Follow ESP-IDF conventions

### Adding Components

1. Create component directory in `components/`
2. Add CMakeLists.txt
3. Implement C HAL and/or C++ API
4. Add Kconfig options if needed

## Safety

⚠️ **Important Safety Notes**:
- Never leave operating system unattended
- Ensure proper ventilation
- Use appropriate temperature for your solder
- Emergency stop button recommended
- Verify work area boundaries before operation

## Troubleshooting

**Cannot connect to WiFi**
- Check SSID/password in menuconfig
- Verify ESP32 is powered and running
- Try rebooting ESP32

**Temperature not reading correctly**
- Check thermocouple connections
- Verify ADC channel configuration
- Calibrate sensor if needed

**Motors not moving**
- Check enable pins (active low)
- Verify power supply (24V)
- Check DRV8825 microstepping configuration

## Contributing

This project was developed as part of the Computer Organization Principles course at Ukrainian Catholic University (UCU).

## Acknowledgments

- Ukrainian Catholic University
- ESP-IDF framework by Espressif
- Course instructor and mentors

---

**Status**: Development  
**Version**: 1.0.0  
**Last Updated**: October 2025

