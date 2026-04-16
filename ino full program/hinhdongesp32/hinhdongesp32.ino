
/*
 * Dasai Mochi - OLED Animation Device
 * ESP32 DevKit + SSD1306/SH1106 128x64 OLED
 *
 * Features:
 * - Random blinking and eye movement
 * - Personality-driven idle behavior
 * - Emotion states and smooth transitions
 * - Sleep/wake power management
 * - WiFi AP control panel for phone/browser control
 */

#include <Arduino.h>
#include "config.h"
#include "all_frames.h"
#include "animations.h"
#include "display_manager.h"
#include "input_handler.h"
#include "state_machine.h"
#if ENABLE_BLE_CONTROL
#include "ble_control.h"
#endif

AnimationEngine animator;
DisplayManager displayMgr;
InputHandler inputHandler;
StateMachine* stateMachine = nullptr;
#if ENABLE_BLE_CONTROL
BleControl bleControl;
#endif

static String serialVoiceBuffer;

void handleSerialVoiceCommands() {
  if (stateMachine == nullptr) {
    return;
  }

  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      serialVoiceBuffer.trim();
      if (serialVoiceBuffer.length() > 0) {
        bool handled = stateMachine->handleVoiceCommand(serialVoiceBuffer);
        Serial.print("Voice cmd: ");
        Serial.print(serialVoiceBuffer);
        Serial.print(" => ");
        Serial.println(handled ? "ok" : "ignored");
      }
      serialVoiceBuffer = "";
      continue;
    }

    if (serialVoiceBuffer.length() < 80) {
      serialVoiceBuffer += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(600);
  randomSeed(ESP.getCycleCount());

  Serial.println();
  Serial.println("====================================");
  Serial.println("  DASAI MOCHI - ESP32 EDITION");
  Serial.println("====================================");

#if DISPLAY_IS_SH1106
  Serial.println("Display driver: SH1106");
#else
  Serial.println("Display driver: SSD1306");
#endif

#if FRAME_DATA_IS_XBM
  Serial.println("Frame mode: XBM (drawXBMP)");
#else
  Serial.println("Frame mode: packed bitmap (drawBitmap)");
#endif

  displayMgr.begin();
  inputHandler.begin();

  stateMachine = new StateMachine(&animator, &displayMgr, &inputHandler);
  stateMachine->begin();

  displayMgr.showSplash("Dasai Mochi", "Alive and watching");
  delay(900);

#if ENABLE_BLE_CONTROL
  bleControl.begin(stateMachine);
#endif

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Current state: ");
  Serial.println(stateMachine->getStateName());
  Serial.print("Current emotion: ");
  Serial.println(stateMachine->getEmotionName());

  delay(500);
  displayMgr.clearDisplay();
}

void loop() {
  handleSerialVoiceCommands();

  if (stateMachine != nullptr) {
    stateMachine->update();
    stateMachine->render();
  }

#if ENABLE_BLE_CONTROL
  bleControl.update();
#endif

  yield();
}
