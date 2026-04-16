#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <driver/i2s.h>
#endif

enum ButtonEvent {
  BUTTON_NONE = 0,
  BUTTON_SHORT_PRESS = 1,
  BUTTON_LONG_PRESS = 2,
  BUTTON_DOUBLE_PRESS = 3
};

enum TouchSide {
  TOUCH_SIDE_NONE = 0,
  TOUCH_SIDE_LEFT = 1,
  TOUCH_SIDE_RIGHT = 2
};

enum TouchEvent {
  TOUCH_EVENT_NONE = 0,
  TOUCH_EVENT_SHORT = 1,
  TOUCH_EVENT_LONG = 2,
  TOUCH_EVENT_DOUBLE = 3
};

class InputHandler {
private:
  int rawState = HIGH;
  int stableState = HIGH;
  int lastRawState = HIGH;
  unsigned long lastDebounceAt = 0;
  unsigned long pressStartedAt = 0;
  unsigned long lastReleaseAt = 0;
  bool longPressReported = false;
  int tapCount = 0;
  ButtonEvent queuedEvent = BUTTON_NONE;
  bool initialized = false;

  int sensorValue = 0;
  int sensorBaseline = 0;
  unsigned long lastSoundEventAt = 0;

#if defined(ARDUINO_ARCH_ESP32)
  static constexpr i2s_port_t micPort = I2S_NUM_0;
  bool micInitialized = false;

  bool beginMic() {
    i2s_config_t i2sConfig = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#if ESP_IDF_VERSION_MAJOR >= 4
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
      .communication_format = I2S_COMM_FORMAT_I2S,
#endif
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0
    };

    i2s_pin_config_t pinConfig = {
      .bck_io_num = 32,
      .ws_io_num = 25,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = 33
    };

    if (i2s_driver_install(micPort, &i2sConfig, 0, nullptr) != ESP_OK) {
      return false;
    }

    if (i2s_set_pin(micPort, &pinConfig) != ESP_OK) {
      i2s_driver_uninstall(micPort);
      return false;
    }

    i2s_zero_dma_buffer(micPort);
    micInitialized = true;
    return true;
  }

  int readMicLevel() {
    if (!micInitialized) {
      return 0;
    }

    static int32_t samples[64];
    size_t bytesRead = 0;
    esp_err_t result = i2s_read(micPort, samples, sizeof(samples), &bytesRead, 0);
    if (result != ESP_OK || bytesRead == 0) {
      return 0;
    }

    int sampleCount = bytesRead / sizeof(int32_t);
    int peak = 0;
    for (int index = 0; index < sampleCount; index++) {
      int32_t rawSample = samples[index] >> 14;
      int level = abs(rawSample);
      if (level > peak) {
        peak = level;
      }
    }

    return peak;
  }
#endif

  int touchLeftRaw = LOW;
  int touchRightRaw = LOW;
  int touchLeftStable = LOW;
  int touchRightStable = LOW;
  int touchLeftLastRaw = LOW;
  int touchRightLastRaw = LOW;
  unsigned long touchLeftDebounceAt = 0;
  unsigned long touchRightDebounceAt = 0;
  TouchSide queuedTouchSide = TOUCH_SIDE_NONE;
  TouchEvent queuedTouchEvent = TOUCH_EVENT_NONE;
  unsigned long touchLeftPressedAt = 0;
  unsigned long touchRightPressedAt = 0;
  unsigned long touchLeftLastReleaseAt = 0;
  unsigned long touchRightLastReleaseAt = 0;
  bool touchLeftLongReported = false;
  bool touchRightLongReported = false;
  uint8_t touchLeftTapCount = 0;
  uint8_t touchRightTapCount = 0;

public:
  void begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
#if TOUCH_ACTIVE_HIGH
    pinMode(TOUCH_LEFT_PIN, INPUT);
    pinMode(TOUCH_RIGHT_PIN, INPUT);
#else
    pinMode(TOUCH_LEFT_PIN, INPUT_PULLUP);
    pinMode(TOUCH_RIGHT_PIN, INPUT_PULLUP);
#endif
  #if defined(ARDUINO_ARCH_ESP32)
    beginMic();
    sensorValue = readMicLevel();
    sensorBaseline = sensorValue;
  #else
    sensorBaseline = analogRead(SENSOR_PIN);
    sensorValue = sensorBaseline;
  #endif
    initialized = true;
  }

  void update() {
    if (!initialized) {
      return;
    }

    unsigned long now = millis();
    rawState = digitalRead(BUTTON_PIN);

    if (rawState != lastRawState) {
      lastDebounceAt = now;
    }

    if ((now - lastDebounceAt) > DEBOUNCE_TIME && rawState != stableState) {
      stableState = rawState;

      if (stableState == LOW) {
        pressStartedAt = now;
        longPressReported = false;
      } else {
        unsigned long pressDuration = now - pressStartedAt;

        if (!longPressReported && pressDuration < LONG_PRESS_TIME) {
          tapCount++;
          lastReleaseAt = now;
        }
      }
    }

    if (stableState == LOW && !longPressReported && (now - pressStartedAt) >= LONG_PRESS_TIME) {
      queuedEvent = BUTTON_LONG_PRESS;
      longPressReported = true;
      tapCount = 0;
    }

    if (tapCount > 0 && stableState == HIGH && (now - lastReleaseAt) >= DOUBLE_TAP_WINDOW) {
      queuedEvent = (tapCount >= 2) ? BUTTON_DOUBLE_PRESS : BUTTON_SHORT_PRESS;
      tapCount = 0;
    }

    // Left touch debouncing and edge capture
    touchLeftRaw = digitalRead(TOUCH_LEFT_PIN);
    if (touchLeftRaw != touchLeftLastRaw) {
      touchLeftDebounceAt = now;
    }
    if ((now - touchLeftDebounceAt) > DEBOUNCE_TIME && touchLeftRaw != touchLeftStable) {
      touchLeftStable = touchLeftRaw;
#if TOUCH_ACTIVE_HIGH
      if (touchLeftStable == HIGH) {
        touchLeftPressedAt = now;
        touchLeftLongReported = false;
      }
#else
      if (touchLeftStable == LOW) {
        touchLeftPressedAt = now;
        touchLeftLongReported = false;
      }
#endif
      else {
        unsigned long touchDuration = now - touchLeftPressedAt;
        if (!touchLeftLongReported && touchDuration < LONG_PRESS_TIME) {
          touchLeftTapCount++;
          touchLeftLastReleaseAt = now;
        }
      }
    }

    if (touchLeftStable == HIGH && !touchLeftLongReported && (now - touchLeftPressedAt) >= LONG_PRESS_TIME) {
      queuedTouchSide = TOUCH_SIDE_LEFT;
      queuedTouchEvent = TOUCH_EVENT_LONG;
      touchLeftLongReported = true;
      touchLeftTapCount = 0;
    }

    if (touchLeftTapCount > 0 && touchLeftStable == LOW && (now - touchLeftLastReleaseAt) >= DOUBLE_TAP_WINDOW) {
      queuedTouchSide = TOUCH_SIDE_LEFT;
      queuedTouchEvent = (touchLeftTapCount >= 2) ? TOUCH_EVENT_DOUBLE : TOUCH_EVENT_SHORT;
      touchLeftTapCount = 0;
    }

    // Right touch debouncing and edge capture
    touchRightRaw = digitalRead(TOUCH_RIGHT_PIN);
    if (touchRightRaw != touchRightLastRaw) {
      touchRightDebounceAt = now;
    }
    if ((now - touchRightDebounceAt) > DEBOUNCE_TIME && touchRightRaw != touchRightStable) {
      touchRightStable = touchRightRaw;
#if TOUCH_ACTIVE_HIGH
      if (touchRightStable == HIGH) {
        touchRightPressedAt = now;
        touchRightLongReported = false;
      }
#else
      if (touchRightStable == LOW) {
        touchRightPressedAt = now;
        touchRightLongReported = false;
      }
#endif
      else {
        unsigned long touchDuration = now - touchRightPressedAt;
        if (!touchRightLongReported && touchDuration < LONG_PRESS_TIME) {
          touchRightTapCount++;
          touchRightLastReleaseAt = now;
        }
      }
    }

    if (touchRightStable == HIGH && !touchRightLongReported && (now - touchRightPressedAt) >= LONG_PRESS_TIME) {
      queuedTouchSide = TOUCH_SIDE_RIGHT;
      queuedTouchEvent = TOUCH_EVENT_LONG;
      touchRightLongReported = true;
      touchRightTapCount = 0;
    }

    if (touchRightTapCount > 0 && touchRightStable == LOW && (now - touchRightLastReleaseAt) >= DOUBLE_TAP_WINDOW) {
      queuedTouchSide = TOUCH_SIDE_RIGHT;
      queuedTouchEvent = (touchRightTapCount >= 2) ? TOUCH_EVENT_DOUBLE : TOUCH_EVENT_SHORT;
      touchRightTapCount = 0;
    }

    lastRawState = rawState;
    touchLeftLastRaw = touchLeftRaw;
    touchRightLastRaw = touchRightRaw;

  #if defined(ARDUINO_ARCH_ESP32)
    sensorValue = readMicLevel();
  #else
    sensorValue = analogRead(SENSOR_PIN);
  #endif

    // Slow adaptive baseline for MAX9814 ambient noise floor
    int delta = sensorValue - sensorBaseline;
    if (abs(delta) < (AUDIO_THRESHOLD / 2)) {
      sensorBaseline = (sensorBaseline * 31 + sensorValue) / 32;
    }
  }

  ButtonEvent consumeEvent() {
    ButtonEvent event = queuedEvent;
    queuedEvent = BUTTON_NONE;
    return event;
  }

  TouchSide consumeTouchSide() {
    TouchSide side = queuedTouchSide;
    queuedTouchSide = TOUCH_SIDE_NONE;
    return side;
  }

  TouchEvent consumeTouchEvent() {
    TouchEvent event = queuedTouchEvent;
    queuedTouchEvent = TOUCH_EVENT_NONE;
    return event;
  }

  bool isButtonPressed() const {
    return stableState == LOW;
  }

  bool isTouchLeftActive() const {
#if TOUCH_ACTIVE_HIGH
    return touchLeftStable == HIGH;
#else
    return touchLeftStable == LOW;
#endif
  }

  bool isTouchRightActive() const {
#if TOUCH_ACTIVE_HIGH
    return touchRightStable == HIGH;
#else
    return touchRightStable == LOW;
#endif
  }

  bool isTouchActive() const {
    return isTouchLeftActive() || isTouchRightActive();
  }

  unsigned long getButtonPressDuration() const {
    if (stableState == LOW) {
      return millis() - pressStartedAt;
    }
    return 0;
  }

  int getSensorValue() const {
    return sensorValue;
  }

  int getSensorDelta() const {
    return abs(sensorValue - sensorBaseline);
  }

  bool detectSound(int threshold = AUDIO_THRESHOLD, unsigned long cooldownMs = 280) {
    unsigned long now = millis();
    if ((now - lastSoundEventAt) < cooldownMs) {
      return false;
    }

    if (getSensorDelta() >= threshold) {
      lastSoundEventAt = now;
      return true;
    }

    return false;
  }

  void updateSensorBaseline() {
    sensorBaseline = sensorValue;
  }
};

#endif // INPUT_HANDLER_H
