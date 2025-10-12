Generate project structure based on the given description. Note that you should create directories and files, with predefined structure, but no implementation. For h/hpp files create classes, structure and documentation, but not implementation. Leave c and cpp files empty, except file with main_app function(enter_point). Files like js and css leave empty. In files like html create basic file structure, but without content. If files was said to be empty, it should not contain any code, but as will be mentioned in next text, it should contain some file documentation. Files like CMakeLists.txt must be already complete and valid, but keep them simple. There is already preconfigured project structure made by platformio, but if you know how to organize project structure better, do it, you have full right to delete/moove/rename folders and move files. Document code, but dont make it look obviously like AI generated, but make it of high quality. Also in c/h/cpp/hpp use good practises for embeded development(e.g. use int16_t or int32_t instead of int, use uint8_t instead of char, dont use float at all, use double instead)
We are creatigng automaticall soldering station, remade from uncomplete self made 3d printer.
Idea:
```
On users computer:
  pcb drill file
   v
  translator(python script)
   v
  our gcode dialect file

1) esp32 open hotspot and host server on it.
2) user connects to hotspot and open web page

Web page:
  1) Field to load gcode
  2) If gcode loaded, image representation of loaded gcode
  3) Panel to manually operate soldering robot
  4) Panel with data about current state of soldering robot(position, work in progress, temperature of soldering tool, time left)
  5) If work in progress, visualization of current state of job(position of soldering tool, previous and next movements)
  6) Start and stop buttons
  7) Additionall parameters panel(temperature of soldering iron, if not set in gcode)

  User load gcode file to web page
   v
  Web page send gcode to esp32

ESP32:
  1) All time run server with web page, handle all inputs from there
  2) If gcode was loaded and command to start was given, doing soldering job(server is still running)
  3) Do job by following gcode commands. Before start, check if gcode is valid(for example: if position is not out of reach)
  4) On small oled display show current position, estimated time to completion of job, time(to show that esp32 is running and not froze) and temperature of soldering tool
  5) Controll all movements of motors(x+y+z movements, soldering supply) and temperature of soldering tool
  
  Programming will be done without using Arduino core, instead using native esp-idf.

Base:
  X-motor + driver - axis of movements of top part with soldering tool
  Y-motor + driver - axis of movements of bottom part, which hold pcb
  Z-motor + driver - axis of movement of soldering tool up and down
  S-motor + driver - motor which supply solder(need to be very precise)
  t(temperature sensor/thermocouple) - read temperature of soldering tool
  soldering tool + driver/transistor - heating element, which will melt solder. Temperature can be const or regulatable.

GCode commands(conceptually):
  Set absolute position(X, Y, Z):
    if Z changed to higher, first perform Z move, if Z changed to lower, first perform XZ move.
  Give soldering supply(N):
    rotate supply motor by N*smallest_available_step(the highest supply resolution)
  Set temperature(T) (if we will need to implemented it):
    set temperature to T
```

Additional info about hardware:
```
II. INFORMATION ABOUT HARDWARE
• DOIT ESP32 DevKit V1 - one of the development
board created by DOIT to evaluate the ESP-WROOM-
32 module. It is based on the ESP32 microcontroller that
boasts Wifi, Bluetooth, Ethernet and Low Power support
all in a single chip. [1]
We have chosen it because it is cheep, powerfull and can
be used to both: controll step motors and host web page.
• DRV8825 Step Motor Driver - driver which allow to
controll step motors with microcontroller. We chosen it
because it support 1/2, 1/4, 1/8, 1/16 and 1/32 microsteps,
which can help us to achive good precision. [3]
• Step Motor 42SHDC3025-24B (0.9A) Anet
This project was developed as part of the ”Computer Organization Princi-
ples” course at the Ukrainian Catholic University (UCU).
Mentor / Project Supervisor
A. Schematic diagram
Some ideas for the schematic shown in Fig. 1 [4] we took
from this video. [5]
• Our project uses a 24V, 3A power supply unit. We use
the 24V to directly power the soldering iron’s heating
element and also to supply a DC-DC buck converter
(LM2596). This module steps the voltage down from 24V
to 5V to power the ESP32 board via its VIN pin.
• To read the temperature, we use an AD8495 K-Type
thermocouple amplifier breakout board. The analog signal
from this module is fed into an ADC pin on the ESP32,
which reads the value and makes necessary adjustments
to the heater.
• Temperature is controlled using a PWM signal gener-
ated by GPIO 22 on the ESP32. This signal drives an
IRLZ44N MOSFET, which acts as a switch, controlling
the current flow to the heating element.
• We also use resistors on the connection to GPIO 22 in
order to protect the pin and to ensure the MOSFET’s gate
is in a defined state (either high or low).
Fig. 1. The schematic diagram for soldering iron control
III. INFORMATION ABOUT SOFTWARE
A. Flash of ESP32
Flash for our microcontroller is written using c++ and ESP-
IDF framework. We have chosen this framework as it is native
development framework for the ESP32 [2].
B. Drill file parser
For converting drill files to gcode we wrote parser using
python.
```
Additional notes:
```
- For configuration data, like pins for motors, soldering iron, etc, Kconfig will be used.
- Simple g code validation will be runned using js on client side.
- There will be no tests.
- LittleFS will be used to save files like index.html, index.js, etc.
- Hardware components: step motors and soldering iron, would have both C HAL and C++ API, where second would use the first.
- ESP32 woudl open hotspot and host web server on it. Web server would be a simple js, css, html server, but some computations like converting drillfile/gerber file to gcode and selecting which holes to solder and which to not(gui). Also some parameters would be availabel to set on hosted web page.
- Additionall functional(like small oled display with some soldering process information) is not confirmed yet, but can be added(it isnt limited to oled with info), so project structure shoudl be extensible.
```
Info about structure:
```
Structure should follow industry and used tools standart. It shoudl be consistent and clear. In addition creeate ProjectStructure.md where explain structure and purpose of each directory and file. In addition in each file(html, js, css, c, cpp, h, hpp, CMakeLists.txt, etc) at the beggining, short documentation of file and its purpose should be provided. This documentation should also follow some industry standart and be consistent, informative and clear.
```