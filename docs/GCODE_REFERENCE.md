# G-Code Dialect Reference

## Overview

This document describes the custom G-Code dialect used by the Automatic Soldering Station. The dialect is based on standard CNC G-Code but tailored for soldering operations.

## Coordinate System

- **Units**: Millimeters (mm)
- **Origin**: Lower-left corner of work area
- **Axes**:
  - X: Horizontal movement of soldering tool
  - Y: Horizontal movement of PCB platform
  - Z: Vertical movement of soldering tool (up/down)

## Supported Commands

### Motion Commands

#### G0 - Rapid Positioning
```gcode
G0 X10.5 Y20.3 Z5.0
```
Moves to specified position at maximum speed. Primarily used for non-soldering moves.

**Parameters:**
- `X` - X-axis position (optional)
- `Y` - Y-axis position (optional)
- `Z` - Z-axis position (optional)
- `F` - Feed rate in mm/min (optional, overrides default)

**Behavior:**
- If Z increases (tool moving up): Z moves first, then X/Y
- If Z decreases (tool moving down): X/Y move first, then Z
- This prevents collisions with work piece

**Example:**
```gcode
G0 X0 Y0 Z10    ; Move to origin with tool raised
G0 Z0.5         ; Lower tool to working height
```

#### G1 - Linear Move
```gcode
G1 X15.0 Y25.0 F100
```
Linear interpolated move at specified feed rate. Used during soldering operations.

**Parameters:**
- `X`, `Y`, `Z` - Target positions
- `F` - Feed rate in mm/min (required for first G1, then sticky)

**Example:**
```gcode
G1 Z0.2 F50     ; Lower to solder height slowly
G1 X10 Y10      ; Move to solder point (uses F50)
```

#### G28 - Home All Axes
```gcode
G28
```
Returns all axes to home position (0, 0, Z_max).

**No parameters required**

**Behavior:**
1. Raises Z to maximum
2. Homes X and Y simultaneously
3. Resets position counters to zero

### Dwell Commands

#### G4 - Dwell (Pause)
```gcode
G4 P1000
```
Pauses execution for specified time.

**Parameters:**
- `P` - Dwell time in milliseconds

**Example:**
```gcode
G1 Z0.1         ; Lower to PCB
G4 P2000        ; Wait 2 seconds for heating
; Feed solder command here
G4 P1000        ; Wait 1 second for solder to flow
G0 Z5           ; Retract
```

### Temperature Commands

#### M104 - Set Hotend Temperature
```gcode
M104 S350
```
Sets soldering iron target temperature.

**Parameters:**
- `S` - Target temperature in Celsius

**Temperature Limits:**
- Minimum: Set in Kconfig (default 200°C)
- Maximum: Set in Kconfig (default 450°C)

**Example:**
```gcode
M104 S350       ; Set to 350°C for standard solder
; Wait for temperature to stabilize
G4 P5000        ; Wait 5 seconds
```

#### M109 - Set Temperature and Wait
```gcode
M109 S350
```
Sets temperature and waits for it to be reached before continuing.

**Parameters:**
- `S` - Target temperature in Celsius

**Example:**
```gcode
M109 S350       ; Set and wait for 350°C
; Proceed with soldering
```

### Custom Soldering Commands

#### S - Feed Solder
```gcode
S100
```
Feeds solder wire by specified amount.

**Parameters:**
- Value (no letter) - Feed amount in steps

**Example:**
```gcode
G1 Z0.1         ; Position at solder point
G4 P500         ; Wait for heat transfer
S50             ; Feed 50 steps of solder
G4 P1000        ; Let solder flow
G0 Z5           ; Retract
```

**Calibration:**
Steps per mm of solder depends on:
- Solder wire diameter
- Motor gear ratio
- Microstepping setting

Typical values: 50-200 steps per mm

## Comment Syntax

```gcode
; This is a comment
G0 X10 Y10  ; Move to position
```

Semicolon (`;`) begins a comment. Everything after is ignored.

## Complete Soldering Sequence Example

```gcode
; Initialize
M109 S350              ; Set temperature and wait
G28                    ; Home all axes

; First solder point
G0 X10.0 Y15.0 Z10.0  ; Move to above first point
G0 Z0.5               ; Lower to approach height
G1 Z0.1 F50           ; Lower slowly to contact
G4 P1000              ; Heat pad
S75                   ; Feed solder
G4 P800               ; Let flow
G0 Z10                ; Retract

; Second solder point
G0 X20.0 Y15.0        ; Move to second point
G0 Z0.5
G1 Z0.1 F50
G4 P1000
S75
G4 P800
G0 Z10

; Finish
G28                   ; Return home
M104 S0               ; Turn off heater
```

## Work Area Boundaries

All movements must stay within configured work area:
- X: 0 to CONFIG_WORK_AREA_X (default 200mm)
- Y: 0 to CONFIG_WORK_AREA_Y (default 200mm)
- Z: 0 to CONFIG_WORK_AREA_Z (default 100mm)

Movements outside these limits will be rejected during validation.

## Validation Rules

The parser validates:
1. **Syntax**: Proper G-Code format
2. **Parameters**: Valid parameter letters and values
3. **Boundaries**: All positions within work area
4. **Temperature**: Within min/max limits
5. **Commands**: Only supported commands

Validation occurs:
- Client-side (JavaScript) before upload
- Server-side (ESP32) before execution

## Feed Rate Guidelines

Recommended feed rates:
- **Rapid moves (G0)**: Maximum (set in config)
- **Approach (G1)**: 100-200 mm/min
- **Lowering to PCB**: 50-100 mm/min
- **Solder point**: 10-50 mm/min

## Temperature Guidelines

| Solder Type | Temperature |
|-------------|-------------|
| Sn60/Pb40 | 300-350°C |
| SAC305 (Lead-free) | 350-400°C |
| Low-temp | 250-300°C |

## Advanced Techniques

### Multiple Points with Loop
```gcode
; (This would require preprocessor - not directly supported)
; Generate with Python script instead
```

### Thermal Relief
```gcode
; For large ground planes, increase temperature
M104 S380           ; Higher temp for ground plane
G0 X50 Y50 Z10
G1 Z0.1 F50
G4 P2000            ; Longer preheat
S100                ; More solder
G4 P1500            ; Longer flow time
G0 Z10
M104 S350           ; Back to normal temp
```

### Drag Soldering
```gcode
; Apply solder while moving
G0 X10 Y10 Z10
G1 Z0.2 F50         ; Lower to drag height
G4 P500
S200                ; Feed significant solder
G1 X20 Y10 F20      ; Drag slowly
G4 P500             ; Finish flowing
G0 Z10
```

## Error Handling

If an error occurs during execution:
1. Motion stops immediately
2. Heater remains at set temperature
3. Error message sent to web interface
4. Manual intervention required

**Common Errors:**
- Out of bounds movement
- Temperature out of range
- Invalid command
- Mechanical limit reached

## Best Practices

1. **Always home first**: Start with `G28`
2. **Set temperature early**: Use `M109` before movements
3. **Safe Z heights**: Keep Z high during X/Y moves
4. **Consistent feed rates**: Set F parameter explicitly
5. **Allow dwell time**: PCB needs time to heat
6. **Test on scrap**: Verify program before production
7. **Add comments**: Document your G-Code

## Generating G-Code

### From Drill Files
```bash
python tools/drill_to_gcode.py input.drl output.gcode
```

### Manual Writing
Use any text editor. Recommended structure:
```gcode
; Header comments
; Date, PCB name, settings

; Initialization
M109 S350
G28

; Solder points
; (one sequence per point)

; Cleanup
G28
M104 S0
```

### CAM Software
Standard PCB CAM tools can export G-Code. You may need to:
- Adjust coordinate system
- Add temperature commands
- Add solder feed commands
- Adjust feed rates

## Debugging

Enable verbose mode for detailed execution logs:
```gcode
; Check parser output in web interface
; Monitor serial output for detailed logs
```

Web interface shows:
- Current line number
- Current command
- Position
- Temperature
- Errors/warnings

---

**Revision**: 1.0  
**Last Updated**: October 2025
