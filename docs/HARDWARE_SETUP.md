# Hardware Setup Guide

## Bill of Materials

### Main Components

| Component | Quantity | Specifications | Notes |
|-----------|----------|---------------|-------|
| ESP32 DOIT DevKit V1 | 1 | ESP-WROOM-32 | Main controller |
| DRV8825 Motor Driver | 4 | Up to 1/32 microstepping | One per axis |
| Stepper Motor | 4 | 42SHDC3025-24B, 0.9A | NEMA 17 size |
| AD8495 Thermocouple Amp | 1 | K-Type, analog output | Temperature sensing |
| IRLZ44N MOSFET | 1 | N-Channel, logic level | Heater control |
| LM2596 Buck Converter | 1 | 24V → 5V, 3A | ESP32 power |
| Power Supply | 1 | 24V, 3A minimum | System power |
| K-Type Thermocouple | 1 | Compatible with AD8495 | Temperature probe |

### Passive Components

| Component | Quantity | Value | Purpose |
|-----------|----------|-------|---------|
| Resistor | 1 | 10kΩ | MOSFET pull-down |
| Resistor | 1 | 1kΩ | MOSFET gate protection |
| Capacitors | 4 | 100µF | Motor driver decoupling |

### Optional Components

| Component | Quantity | Specifications | Notes |
|-----------|----------|---------------|-------|
| OLED Display | 1 | 128x64, I2C, SSD1306 | Status display |
| Emergency Stop Button | 1 | Normally closed | Safety |

## Wiring Diagram

### Power Distribution

```
24V PSU (+) ─┬─> Soldering Iron Heater (+)
             ├─> DRV8825 VMOT (all 4 drivers)
             └─> LM2596 VIN
             
24V PSU (-) ─┴─> All GND connections

LM2596 VOUT (+5V) ─> ESP32 VIN
LM2596 GND ───────> ESP32 GND
```

### ESP32 GPIO Connections

#### X-Axis Motor (DRV8825)
- GPIO 25 → STEP
- GPIO 26 → DIR
- GPIO 27 → EN̅

#### Y-Axis Motor (DRV8825)
- GPIO 14 → STEP
- GPIO 12 → DIR
- GPIO 13 → EN̅

#### Z-Axis Motor (DRV8825)
- GPIO 33 → STEP
- GPIO 32 → DIR
- GPIO 15 → EN̅

#### Solder Supply Motor (DRV8825)
- GPIO 18 → STEP
- GPIO 19 → DIR
- GPIO 21 → EN̅

#### Shared Microstepping Pins (All DRV8825)
- GPIO 16 → MODE0 (M0)
- GPIO 17 → MODE1 (M1)
- GPIO 5 → MODE2 (M2)

#### Soldering Iron Control
```
GPIO 22 ──> 1kΩ resistor ──> IRLZ44N Gate
                             IRLZ44N Source ──> GND
                             IRLZ44N Drain ──> Heater (-)
10kΩ resistor: Gate to GND (pull-down)
```

#### Temperature Sensor
- AD8495 VOUT → GPIO 36 (ADC1_CH0)
- AD8495 VCC → 3.3V
- AD8495 GND → GND
- Thermocouple → AD8495 terminals

#### Optional OLED Display
- SDA → GPIO 23
- SCL → GPIO 22 (shared with PWM - check compatibility)
- VCC → 3.3V
- GND → GND

### DRV8825 Connections (Each Driver)

```
VMOT ──> 24V
GND ──> Ground
1A, 1B ──> Motor Coil 1
2A, 2B ──> Motor Coil 2
STEP ──> ESP32 GPIO
DIR ──> ESP32 GPIO
EN̅ ──> ESP32 GPIO
M0, M1, M2 ──> ESP32 GPIO (shared)
RST ──> SLEEP (tied together)
```

**Important**: Add 100µF capacitor between VMOT and GND for each driver.

## Assembly Instructions

### Step 1: Prepare Components
1. Test ESP32 board separately
2. Verify all DRV8825 drivers
3. Check motor coil resistances
4. Test power supply output

### Step 2: Build Power Section
1. Connect LM2596 to 24V input
2. Adjust output to exactly 5V
3. Add filter capacitor on output
4. Test under load (500mA minimum)

### Step 3: Mount Motor Drivers
1. Install DRV8825 on breadboard/PCB
2. Add 100µF capacitors near each VMOT pin
3. Connect all grounds together
4. Wire VMOT to 24V rail
5. Tie RST and SLEEP together on each driver

### Step 4: Wire ESP32 to Drivers
1. Connect all STEP pins
2. Connect all DIR pins
3. Connect all EN̅ pins
4. Wire shared MODE pins
5. Double-check all connections

### Step 5: Connect Motors
1. Identify motor coil pairs (measure resistance)
2. Connect coils to 1A/1B and 2A/2B
3. Label each motor (X, Y, Z, S)
4. Verify no shorts to ground

### Step 6: Temperature System
1. Mount AD8495 module
2. Connect thermocouple
3. Wire VOUT to GPIO 36
4. Connect power (3.3V) and ground
5. Verify output voltage at room temperature

### Step 7: Heater Control
1. Mount MOSFET with heatsink
2. Add gate resistor (1kΩ)
3. Add pull-down resistor (10kΩ)
4. Connect PWM from GPIO 22
5. Wire heater through MOSFET drain
6. **TEST WITH MULTIMETER BEFORE POWER**

### Step 8: Optional Display
1. Connect I2C OLED
2. Verify address (usually 0x3C)
3. Test with I2C scanner sketch

### Step 9: Final Checks
1. Visual inspection of all connections
2. Continuity test for grounds
3. Check for shorts on power rails
4. Verify GPIO assignments match code
5. Test each subsystem individually

## Microstepping Configuration

DRV8825 microstepping is set via MODE pins:

| M0 | M1 | M2 | Microstep Resolution |
|----|----|----|---------------------|
| 0  | 0  | 0  | Full step |
| 1  | 0  | 0  | Half step |
| 0  | 1  | 0  | 1/4 step |
| 1  | 1  | 0  | 1/8 step |
| 0  | 0  | 1  | 1/16 step |
| 1  | 0  | 1  | 1/32 step |
| 0  | 1  | 1  | 1/32 step |
| 1  | 1  | 1  | 1/32 step |

Default configuration uses 1/16 microstepping for good balance of precision and speed.

## Safety Precautions

⚠️ **DANGER - HIGH TEMPERATURE**
- Soldering iron reaches 450°C
- Do not touch heater element
- Allow cooling before handling

⚠️ **ELECTRICAL SAFETY**
- 24V can cause burns
- Verify all connections before power
- Use fuses on power inputs
- Add emergency stop button

⚠️ **MECHANICAL SAFETY**
- Motors can move unexpectedly
- Keep hands clear during operation
- Secure all mechanical parts
- Test movements at low speed first

## Calibration

### Temperature Calibration
1. Measure known temperature with reference thermometer
2. Compare to system reading
3. Adjust offset in code if needed
4. Verify across temperature range

### Motion Calibration
1. Mark known distance on each axis
2. Command movement of exact distance
3. Measure actual movement
4. Adjust STEPS_PER_MM in Kconfig
5. Repeat until accurate

### Solder Feed Calibration
1. Mark solder wire at known position
2. Command specific feed amount
3. Measure actual feed
4. Calculate steps per mm
5. Update configuration

## Troubleshooting

### Motors vibrate but don't move
- Check microstepping configuration
- Verify motor current limit on DRV8825
- Ensure RST and SLEEP are tied high

### Heater not heating
- Check MOSFET connections
- Verify PWM signal with oscilloscope
- Test heater element resistance (should be ~15-30Ω)
- Check 24V supply to heater

### Inaccurate temperature readings
- Check thermocouple polarity
- Verify AD8495 connections
- Test at known temperature
- Check for electrical noise

### WiFi not starting
- Verify ESP32 power (5V stable)
- Check serial monitor for errors
- Reflash firmware
- Try different WiFi channel

## Recommended Tools

- Multimeter
- Oscilloscope (for debugging PWM)
- Crimping tools
- Soldering iron (for assembly)
- Heat shrink tubing
- Wire strippers
- Screwdrivers
- Calipers (for calibration)

## Maintenance

- **Weekly**: Check all connection tightness
- **Monthly**: Clean dust from motors and electronics
- **Quarterly**: Verify temperature calibration
- **As needed**: Replace thermocouple if degraded

---

**Last Updated**: October 2025
