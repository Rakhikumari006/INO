# DASAI MOCHI - ESP8266 OLED Animation Device
## v1.0 - Fully Functional with Personality

### Overview
A complete, modular OLED animation system for ESP8266 that replicates the personality and behavior of a real Dasai Mochi character. Features state machine, multiple emotions, button interaction, and power optimization.

### Hardware Requirements
- **Microcontroller**: ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- **Display**: SSD1306 OLED 128x64 pixels
- **Interface**: I2C (SDA=D2/GPIO4, SCL=D1/GPIO5)
- **Button**: Tactile push button connected to D3 (GPIO0) with GND
- **I2C Pull-ups**: 4.7kΩ resistors on SDA/SCL (usually built-in on ESP8266)

### System Architecture

#### 1. **Animation Engine** (animations.h)
- Manages frame sequences and emotion states
- Supports multiple emotional expressions:
  - **IDLE_NEUTRAL**: Default looping animation (frames 0-100)
  - **HAPPY**: Joyful expression (frames 101-200)
  - **SLEEPY**: Tired/sleepy state (frames 201-300)
  - **SURPRISED**: Shocked reaction (frames 301-350)
  - **BLINK**: Natural blinking (frames 351-370)
  - **CONFUSED**: Puzzled expression (frames 371-420)

- Features:
  - Frame range management
  - Speed control per emotion (20-60ms per frame)
  - Loop or one-shot animation modes
  - Progress tracking (0-100%)
  - Random blinking intervals

#### 2. **Display Manager** (display_manager.h)
- I2C initialization and management
- Buffer control for smooth rendering
- Power optimization:
  - **Normal brightness**: 255 (full)
  - **Sleep brightness**: 50 (dimmed)
  - Power save mode support
- Activity tracking for sleep timeout
- Helper methods for splash screens and status info

#### 3. **Input Handler** (input_handler.h)
- Button input with debouncing (50ms)
- Detects multiple button events:
  - SHORT_PRESS: < 1 second (change emotion)
  - LONG_PRESS: > 1 second (sleep mode)
  - DOUBLE_PRESS: Fast consecutive presses (reserved)
- Supports optional motion/light sensor input (analog pin A0)
- Press duration tracking

#### 4. **State Machine** (state_machine.h)
Manages device operation with the following states:

```
┌─────────────────────────────────────────────────┐
│            STATE TRANSITIONS                     │
├─────────────────────────────────────────────────┤
│                                                  │
│  IDLE ──(30s inactivity)──► SLEEP               │
│   ↑                           ↓                  │
│   └──(button press)──► WAKE ──┘                 │
│    ↓                                             │
│  EMOTION ──(4s)──► IDLE                         │
│    ↓                                             │
│  INTERACT ──(3s)──► IDLE                        │
│                                                  │
└─────────────────────────────────────────────────┘
```

**IDLE State**:
- Default looping animation
- Listens for button input
- Random blinking every 5-10 seconds
- Auto-sleeps after 30 seconds inactivity

**SLEEP State**:
- Display dimmed to 50% brightness
- Slower animation (sleepy emotion)
- Any button press wakes device
- Optional motion sensor trigger

**WAKE State**:
- Plays surprised animation
- Returns to IDLE after 2 seconds
- Full brightness restored

**INTERACT State**:
- Happy emotion display
- 3-second duration

**EMOTION State**:
- Displays selected emotion
- 4-second duration
- Button press cycles through emotions

### Software Features

#### Memory Optimization
- Frame data stored in PROGMEM (flash memory, not RAM)
- Efficient object-oriented design
- Minimal RAM footprint (~2KB for objects)
- Free heap monitoring in serial output

#### Button Control
```
Short Press (< 1s):
  - Changes to random emotion
  - Returns to IDLE after animation

Long Press (> 1s):
  - Activates SLEEP mode
  - Dims display to 50%
  - Returns to IDLE on next button press

(Optional) Motion Sensor:
  - Can wake device from sleep
  - Requires analog sensor on A0
```

#### Customization Points

**Change sleep timeout** (state_machine.h):
```cpp
unsigned long sleepTimeout = 30000;  // milliseconds
```

**Adjust frame ranges** (animations.h):
```cpp
case HAPPY:
  currentStartFrame = 101;
  currentEndFrame = 200;  // Edit these frame numbers
  break;
```

**Modify animation speeds** (animations.h):
```cpp
animationSpeed = 40;  // milliseconds per frame (lower = faster)
```

**Change sleep brightness** (display_manager.h):
```cpp
setSleepBrightness(50);  // 0-255 range
```

### Building and Uploading

#### Method 1: Using UPLOAD.bat (Windows)
```bash
cd "e:\collage\ino\ino full program\hinhdongesp32"
UPLOAD.bat
```

#### Method 2: Arduino IDE
1. Open `hinhdongesp32.ino` in Arduino IDE
2. Select Board: "NodeMCU 1.0 (ESP8266)"
3. Select Port: COM port of your ESP8266
4. Click Upload

#### Method 3: Platform IO
```bash
platformio run -e esp8266 --target upload
```

### Serial Monitor Output
After upload, open Serial Monitor (115200 baud) to see:
```
================================
  DASAI MOCHI v1.0
  ESP8266 OLED Animation Device
================================

Display initialized: 128x64 SSD1306 @ 0x3C
Input Handler initialized
Button on pin D3 (GPIO0)
Free heap: 48360 bytes

DASAI MOCHI Ready!
Button pin: D3 (GPIO0)
- Short press: Change emotion
- Long press: Sleep mode
- Double press: (reserved)
```

State transitions will also be logged:
```
State transition: IDLE -> EMOTION
State transition: EMOTION -> IDLE
State transition: IDLE -> SLEEP
State transition: SLEEP -> WAKE
```

### Advanced Features & Extensions

#### WiFi + NTP Clock (Coming Soon)
```cpp
// For future implementation
#include <time.h>
void syncNTP() {
  configTime(0, 0, "pool.ntp.org");
}
```

#### Sound-Reactive Animations
Connect microphone to A0:
```cpp
int audioLevel = analogRead(A0);
if (audioLevel > THRESHOLD) {
  animator.setEmotion(HAPPY);
}
```

#### Motion Detection
Use PIR sensor on D4:
```cpp
#define MOTION_PIN D4
if (digitalRead(MOTION_PIN) == HIGH) {
  stateMachine->setState(STATE_WAKE);
}
```

#### Custom Emotions
Add new emotion frames to `animations.h`:
```cpp
case NERVOUS:
  currentStartFrame = 421;
  currentEndFrame = 470;
  animationSpeed = 35;
  isLooping = true;
  break;
```

### File Structure
```
hinhdongesp32/
├── hinhdongesp32.ino       # Main program
├── all_frames.h            # All 772 animation frames
├── frames/                 # Individual frame files (0-771)
│   ├── frame_00000.h
│   ├── frame_00001.h
│   └── ... (770 more)
├── animations.h            # Animation engine
├── display_manager.h       # Display control
├── input_handler.h         # Button/sensor input
├── state_machine.h         # State management
├── UPLOAD.bat             # Build script
└── README.md              # This file
```

### Troubleshooting

**Display not showing:**
- Verify I2C address: 0x3C (run I2C_SCANNER.ino)
- Check SDA/SCL connections (D2/D1)
- Verify Wire.begin() is called before display operations

**Button not responding:**
- Verify D3 pin is connected to GND through button
- Check debounce time in input_handler.h
- Serial monitor should show button events

**Animations playing too fast/slow:**
- Adjust `animationSpeed` values in animations.h
- Lower value = faster, higher value = slower
- IDLE default: 42ms per frame (~24 FPS)

**Out of memory:**
- Check free heap in serial monitor
- Reduce frame ranges if needed
- Ensure frames are stored in PROGMEM only

### Performance Metrics
- **Free RAM**: ~48KB available
- **Frame Rate**: Up to 50 FPS (adjustable)
- **Power Draw**: ~100mA normal, ~20mA sleep
- **I2C Clock**: 400kHz
- **Display Update**: ~20ms per frame

### Future Enhancements
- [ ] WiFi connectivity for time sync
- [ ] SD card support for custom animations
- [ ] EEPROM settings (brightness, timeouts)
- [ ] Bluetooth remote control
- [ ] Gesture recognition
- [ ] Multiple display layout modes
- [ ] Animation sequencing/scripting

### License
Educational/Personal use

### Credits
- Original animation concept by @khoi2mai
- U8g2 library by Oliver Kraus
- ESP8266 Arduino core
- SSD1306 OLED driver

---

**Happy Mochi-ing!** 🎌
