/*
 * EXAMPLES.H - Code Examples for Customization
 * 
 * This file contains example code snippets showing how to extend
 * and customize the Dasai Mochi system.
 * 
 * Copy and paste these examples into your code as needed.
 */

#ifndef EXAMPLES_H
#define EXAMPLES_H

// ============================================
// EXAMPLE 1: Add Custom Emotion
// ============================================

/*
In animations.h, in the AnimationEngine::setEmotion() method:

  case NERVOUS:
    currentStartFrame = 421;      // Adjust to your frame range
    currentEndFrame = 470;
    animationSpeed = 35;          // Faster = lower number
    isLooping = true;             // Loops continuously
    break;

Then in state_machine.h, call it like:
  animator->setEmotion(NERVOUS);
*/

// ============================================
// EXAMPLE 2: Add Motion Sensor Wake
// ============================================

/*
In state_machine.h, in handleSleepState():

  // Wake on motion sensor
  if (input->getSensorValue() > MOTION_THRESHOLD) {
    setState(STATE_WAKE);
    display->recordActivity();
    return;
  }

Make sure to enable in config.h:
  #define ENABLE_MOTION_SENSOR 1
*/

// ============================================
// EXAMPLE 3: Add Custom Button Behavior
// ============================================

/*
In state_machine.h, add a case in update() method:

  case STATE_CUSTOM:
    handleCustomState(elapsedInState, btnEvent);
    break;

Then add the handler:

  void handleCustomState(unsigned long elapsed, ButtonEvent btnEvent) {
    if (btnEvent == BUTTON_SHORT_PRESS) {
      // Do something custom
      animator->setEmotion(HAPPY);
    }
    
    if (elapsed >= 5000) {  // Timeout after 5 seconds
      setState(STATE_IDLE);
    }
  }
*/

// ============================================
// EXAMPLE 4: Add WiFi Time Display
// ============================================

/*
Create a new header file: ntp_time.h

#include <configTime.h>
#include <time.h>

class NTPTime {
public:
  void begin() {
    configTime(0, 0, "pool.ntp.org");
  }
  
  String getTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M", timeinfo);
    return String(buffer);
  }
};

Then in hinhdongesp32.ino:
  #include "ntp_time.h"
  NTPTime ntp;
  
  And in setup():
    ntp.begin();
  
  And in loop(), display time when needed:
    display->drawStr(100, 50, ntp.getTime().c_str());
*/

// ============================================
// EXAMPLE 5: Add Sound Reactivity
// ============================================

/*
In input_handler.h, add audio analysis:

  class InputHandler {
    private:
      int audioLevel = 0;
      unsigned long lastAudioTime = 0;
      
    public:
      void updateAudio() {
        audioLevel = analogRead(SENSOR_PIN);
      }
      
      bool isSoundDetected() {
        return audioLevel > AUDIO_THRESHOLD;
      }
  };

In state_machine.h, handleIdleState():

  if (input->isSoundDetected()) {
    animator->setEmotion(HAPPY);
    setState(STATE_EMOTION);
  }
*/

// ============================================
// EXAMPLE 6: Add Gesture Recognition
// ============================================

/*
In input_handler.h, add double/triple press detection:

  int pressCount = 0;
  unsigned long lastPressTime = 0;
  
  ButtonEvent getButtonEvent() {
    if (buttonReleased) {
      pressCount++;
      lastPressTime = millis();
      
      // Wait for more presses or timeout
      if (millis() - lastPressTime > 500) {
        if (pressCount >= 3) return TRIPLE_PRESS;
        if (pressCount >= 2) return DOUBLE_PRESS;
        pressCount = 0;
      }
    }
  }

In state_machine.h:

  case BUTTON_TRIPLE_PRESS:
    animator->setEmotion(SURPRISED);
    setState(STATE_EMOTION);
    break;
*/

// ============================================
// EXAMPLE 7: Add Memory Monitoring
// ============================================

/*
In hinhdongesp32.ino, in loop():

  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 10000) {  // Every 10 seconds
    lastMemCheck = millis();
    uint32_t freeHeap = ESP.getFreeHeap();
    Serial.print("Free heap: ");
    Serial.println(freeHeap);
    
    if (freeHeap < 20000) {  // Less than 20KB free
      Serial.println("WARNING: Low memory!");
    }
  }
*/

// ============================================
// EXAMPLE 8: Add EEPROM Settings
// ============================================

/*
#include <EEPROM.h>

class Settings {
private:
  const int EEPROM_SIZE = 512;
  
public:
  void begin() {
    EEPROM.begin(EEPROM_SIZE);
  }
  
  void saveBrightness(int brightness) {
    EEPROM.put(0, brightness);
    EEPROM.commit();
  }
  
  int loadBrightness() {
    int brightness;
    EEPROM.get(0, brightness);
    return constrain(brightness, 0, 255);
  }
};

In setup():
  Settings settings;
  settings.begin();
  displayMgr.setBrightness(settings.loadBrightness());
*/

// ============================================
// EXAMPLE 9: Create Animation Sequence
// ============================================

/*
Create a method to chain animations:

void playSequence() {
  // First: Surprised (0.5s)
  animator->setEmotion(SURPRISED);
  delay(500);
  
  // Then: Confused (1s)
  animator->setEmotion(CONFUSED);
  delay(1000);
  
  // Finally: Happy (1s)
  animator->setEmotion(HAPPY);
  delay(1000);
  
  // Back to idle
  animator->setEmotion(IDLE_NEUTRAL);
}

Call from state machine:
  if (someCondition) {
    playSequence();
  }
*/

// ============================================
// EXAMPLE 10: Add Speech/Text Display
// ============================================

/*
In display_manager.h:

  void drawText(const char* text, int x, int y) {
    display.clearBuffer();
    display.setFont(u8g2_font_5x8_tf);
    display.drawStr(x, y, text);
    display.sendBuffer();
  }

In state_machine.h:

  void showMessage(const char* msg) {
    display->drawText(msg, 10, 30);
    delay(2000);
    clearDisplay();
  }
*/

// ============================================
// COMPILATION NOTES
// ============================================

/*
All examples are compatible with:
- ESP8266 Arduino Core
- U8g2lib 2.28+
- Arduino IDE 1.8+

To use examples:
1. Uncomment or copy the code
2. Adjust pin numbers if needed
3. Update config.h with new settings
4. Compile and upload
5. Check serial monitor for output

Memory considerations:
- String literals use PROGMEM when possible
- Frame data already in PROGMEM
- Typical free RAM: 40-50KB
*/

#endif // EXAMPLES_H
