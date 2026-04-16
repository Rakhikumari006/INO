# DASAI MOCHI - TESTING CHECKLIST & VALIDATION GUIDE

## Pre-Compilation Checklist

- [ ] All 772 frame files present (frame_00000.h to frame_00771.h)
- [ ] all_frames.h compiles without errors
- [ ] animations.h syntax correct
- [ ] display_manager.h includes U8g2lib.h
- [ ] input_handler.h pin definitions match hardware
- [ ] state_machine.h logic compiles
- [ ] config.h has no duplicate definitions
- [ ] hinhdongesp32.ino includes all headers
- [ ] PROGMEM keyword used correctly for frame data
- [ ] No undefined function calls

## Compilation Test

### Step 1: Compile the Code
```bash
# Option A: Using UPLOAD.bat
cd e:\collage\ino\ino full program\hinhdongesp32
UPLOAD.bat

# Option B: Arduino IDE
1. Open hinhdongesp32.ino
2. Sketch → Verify/Compile
3. Wait for completion
```

### Expected Results
```
✓ No compilation errors
✓ Sketch uses ~1.0MB flash memory
✓ Heap estimate shows ~40-50KB available
✓ No warnings about PROGMEM
✓ Build time: 5-30 seconds depending on hardware
```

## Hardware Verification

### Display Connection Test
```
Verify I2C connection:
1. No display? Check wiring:
   - SDA (Display) → D2 (ESP8266)
   - SCL (Display) → D1 (ESP8266)
   - GND (Display) → GND (ESP8266)
   - VCC (Display) → 3.3V (ESP8266)

2. No display but compilation OK? 
   → Run I2C_SCANNER.ino to verify:
   - Display appears at 0x3C?
   - Display address correct?
   - I2C bus responds?
```

### Button Connection Test
```
1. Button not responding? Check:
   - Button pin D3 connected to GND?
   - Pull-up enabled in code?
   - Serial shows "Button on pin D3"?

2. Button debounce issues? Adjust:
   #define DEBOUNCE_TIME 50  // in input_handler.h or config.h
```

## Upload Test

### Step 1: Upload Code
```bash
UPLOAD.bat

# Expected output:
Compiling sketch...
[████████████] 100%
Uploading code...
[████████████] 100%
Upload complete!
```

### Step 2: Monitor Serial Output
```
1. Open Serial Monitor (115200 baud)
2. ESP8266 reboots (watch for reset message)
3. Should see:

================================
  DASAI MOCHI v1.0
  ESP8266 OLED Animation Device
================================

Display initialized: 128x64 SSD1306 @ 0x3C
Input Handler initialized
Button on pin D3 (GPIO0)
Free heap: 48360 bytes

DASAI MOCHI Ready!
```

## Functional Testing

### Test 1: Display Shows Animation
- [ ] **What to look for**: Animated frames should display on OLED
- [ ] **Expected**: Smooth animation looping
- [ ] **Duration**: Continuous until sleep
- [ ] **Speed**: Should be ~24 FPS (adjustable)
- [ ] **If fails**: Check display connection, verify I2C address

### Test 2: Random Blinking
- [ ] **What to look for**: Brief blinking animations interspersed
- [ ] **Expected**: Random blink every 5-10 seconds
- [ ] **Pattern**: Should be different intervals each time
- [ ] **If fails**: Check BLINK frame range in config.h

### Test 3: Button Short Press
```
Action: Press button for < 1 second (then release)

Expected behavior:
1. Animation changes to emotion (Happy/Sleepy/Surprised/Confused)
2. Serial shows button press
3. Emotion displays for ~4 seconds
4. Returns to IDLE/neutral animation
5. Button press repeats cycle

□ Animation changes  ✓ ________
□ Serial output shows "Button SHORT_PRESS"  ✓ ________
□ Emotion displays for ~4 seconds  ✓ ________
□ Cycles back to IDLE  ✓ ________
```

### Test 4: Button Long Press
```
Action: Hold button for > 1 second (then release)

Expected behavior:
1. Display dims to 50% brightness
2. Animation becomes SLEEPY (slower)
3. Serial shows "State transition: IDLE -> SLEEP"
4. Any button press wakes up
5. Display brightens, shows SURPRISED briefly
6. Returns to normal animation

□ Display dims  ✓ ________
□ Animation slows  ✓ ________
□ Serial shows SLEEP state  ✓ ________
□ Button press wakes  ✓ ________
□ Shows SURPRISED animation  ✓ ________
```

### Test 5: Auto-Sleep (30 seconds)
```
Action: Let device sit idle without button press for 30+ seconds

Expected behavior:
1. No button press for exactly 30 seconds
2. Display automatically dims to 50%
3. Animation changes to SLEEPY
4. Serial shows "State transition: IDLE -> SLEEP"
5. Continues running in sleep mode
6. Button press wakes it

Note: 30 seconds = 30000ms (adjustable in config.h)

□ Auto-sleep triggers at 30s  ✓ ________
□ Display dims automatically  ✓ ________
□ Serial shows transition  ✓ ________
□ Button wakes from sleep  ✓ ________
```

### Test 6: Multiple Emotions
```
Action: Press button multiple times in succession

Expected behavior:
1. First press → Random emotion #1 (4 seconds)
2. Return to IDLE
3. Second press → Different random emotion #2
4. Return to IDLE
5. Emotions should vary (Happy, Sleepy, Surprised, Confused)

Track emotions shown:
Press 1: _______________  Duration: ___s
Press 2: _______________  Duration: ___s
Press 3: _______________  Duration: ___s
Press 4: _______________  Duration: ___s

□ Emotions vary  ✓ ________
□ Timing consistent (~4s)  ✓ ________
□ Transitions smooth  ✓ ________
```

### Test 7: Serial Output Accuracy
```
Serial Monitor should show (115200 baud):

□ Startup splash with version  ✓ ________
□ Display init message  ✓ ________
□ Input handler init message  ✓ ________
□ Free heap value (should be ~48KB)  ✓ ________
□ Ready message with button info  ✓ ________
□ State transitions when states change  ✓ ________
□ No error messages  ✓ ________
```

Example state transitions:
```
State transition: IDLE -> EMOTION
State transition: EMOTION -> IDLE
State transition: IDLE -> SLEEP
State transition: SLEEP -> WAKE
State transition: WAKE -> IDLE
```

## Performance Testing

### Memory Usage
```cpp
// In serial output:
Free heap: XXXXX bytes

Should show:
- Initial: ~48-50KB free
- While running: Stays stable
- No gradual decrease (no memory leak)

If decreasing: Check for memory leaks in loops
```

### Frame Rate
```
Visual test:
1. Observe animation smoothness
2. Should be ~24 FPS in IDLE (fluent)
3. Should be smoother in EMOTION (20-45 FPS)
4. Should be slower in SLEEP (slower, ~16 FPS)

□ IDLE: Smooth, fluent  ✓ ________
□ EMOTION: Very smooth  ✓ ________
□ SLEEP: Noticeably slower  ✓ ________
```

### Button Response Time
```
Visual test:
1. Press button
2. Animation should change almost instantly
3. No lag between press and response
4. Serial output within 50ms

□ Instant response  ✓ ________
□ No visible lag  ✓ ________
□ Serial confirms < 100ms  ✓ ________
```

## Advanced Testing

### Test 8: Stress Test (30 minutes continuous)
```
1. Let device run for 30 minutes
2. Randomly press button every 10-20 seconds
3. Observe for:
   - Memory leaks (free heap decreasing)
   - Frame rate degradation
   - Button responsiveness decline
   - Display degradation

Results:
Start heap: _______ bytes
End heap:   _______ bytes
Difference: _______ bytes

□ No memory leak (difference < 1KB)  ✓ ________
□ Frame rate consistent  ✓ ________
□ Button still responsive  ✓ ________
□ Display still crisp  ✓ ________
```

### Test 9: Power Consumption
```
Optional - Use USB power meter:

Normal mode:
- Button presses: ~100-120mA
- Idle looping: ~80-100mA

Sleep mode:
- Dimmed display: ~30-50mA
- Should be significantly lower than normal

□ Normal current measured  ✓ ________
□ Sleep current lower  ✓ ________
□ No abnormal spikes  ✓ ________
```

### Test 10: Custom Configuration
```
Edit config.h and test:

Test setting: Sleep timeout to 10 seconds
1. Edit: #define SLEEP_TIMEOUT 10000
2. Recompile and upload
3. Wait 10 seconds without pressing button
4. Should auto-sleep at 10s mark (not 30s)

□ Custom timeout works  ✓ ________
□ Takes effect immediately  ✓ ________

Test setting: Change emotion speed
1. Edit: #define IDLE_SPEED 20
2. Recompile and upload
3. Idle animation should be much faster

□ Animation speed changes  ✓ ________
□ Update takes effect  ✓ ________
```

## Troubleshooting Guide

### Issue: Display stays blank
```
Diagnostic steps:
1. Check I2C address: Run I2C_SCANNER.ino
   - Does it find device at 0x3C?
   
2. Check wiring:
   - SDA: D2 (GPIO4) correct?
   - SCL: D1 (GPIO5) correct?
   - GND connected?
   - 3.3V on VCC?
   
3. Check initialization:
   - Serial shows "Display initialized" message?
   - "128x64 SSD1306 @ 0x3C" printed?
   
4. Check contrast:
   - Try: display.setContrast(255) - max brightness
   - Is display actually black or just dim?
```

### Issue: Button not responding
```
Diagnostic steps:
1. Check wiring:
   - Button to D3 (GPIO0)?
   - Button to GND?
   - Not using D0 for anything else?
   
2. Check serial output:
   - Shows "Button on pin D3" message?
   - Shows button events on press?
   
3. Test debounce:
   - Increase: #define DEBOUNCE_TIME 100
   - Try again with slower presses
```

### Issue: Animation too fast/slow
```
Diagnostic steps:
1. Current speed is in config.h
2. IDLE_SPEED default: 42ms per frame
3. To speed up: reduce number (e.g., 20)
4. To slow down: increase number (e.g., 60)
5. Recompile and upload
6. Changes take effect immediately
```

### Issue: Memory issues or crashes
```
Diagnostic steps:
1. Check free heap in serial:
   - Should be ~48KB available
   - If < 10KB, memory problem
   
2. Reduce frame ranges:
   - Use fewer frames for emotions
   - Don't load all 772 frames
   
3. Disable serial debug:
   - #define ENABLE_SERIAL_DEBUG 0
   - Saves ~2KB
```

## Final Validation Checklist

### Core Functionality
- [ ] Display shows animations (smooth)
- [ ] Button changes emotion (responsive)
- [ ] Auto-sleep works (30 seconds)
- [ ] Wake-up animation plays (surprised)
- [ ] Random blinking occurs
- [ ] State transitions logged

### Display Quality
- [ ] Animation is smooth (no flicker)
- [ ] Brightness appropriate (not too dim)
- [ ] Sleep mode visibly dimmer
- [ ] Characters/text crisp (if displayed)
- [ ] Transitions between frames smooth

### User Interaction
- [ ] Button presses always register
- [ ] No lag between press and response
- [ ] Debouncing works (no double-triggers)
- [ ] Long press consistently detected
- [ ] Multiple emotions display correctly

### Stability
- [ ] No crashes or resets
- [ ] Memory usage stable
- [ ] Frame rate consistent
- [ ] Serial output continuous
- [ ] Device runs indefinitely

### Power/Performance
- [ ] Normal mode current reasonable
- [ ] Sleep mode uses less power
- [ ] Frame rate smooth in all modes
- [ ] Animations transition seamlessly
- [ ] No stuttering or lag

## Sign-Off

```
Device Name: DASAI MOCHI v1.0
Hardware: ESP8266 + SSD1306 OLED
Test Date: _______________
Tester: _______________

Overall Status: _______ PASS / FAIL

Issues Found:
_________________________________________
_________________________________________

Recommendations:
_________________________________________
_________________________________________

Signature: _________________ Date: _______
```

---

**Happy Testing!** 🎌
Once all tests pass, your Dasai Mochi is ready for deployment! 🎉
