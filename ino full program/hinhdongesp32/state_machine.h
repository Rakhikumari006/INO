#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "config.h"
#include "animations.h"
#include "display_manager.h"
#include "input_handler.h"

enum DeviceState {
  STATE_IDLE = 0,
  STATE_INTERACT = 1,
  STATE_SLEEP = 2,
  STATE_WAKE = 3,
  STATE_EMOTION_OVERRIDE = 4
};

class StateMachine {
private:
  DeviceState currentState = STATE_IDLE;
  DeviceState previousState = STATE_IDLE;
  unsigned long stateStartedAt = 0;
  unsigned long nextIdlePulseAt = 0;
  unsigned long nextCuriosityAt = 0;
  unsigned long nextEmotionShiftAt = 0;
  Emotion overrideEmotion = IDLE_NEUTRAL;
  bool overrideActive = false;
  bool redrawRequested = true;
  Emotion lastRandomEmotion = HAPPY;
  bool frameAdvancedThisTick = false;
  unsigned long lastInteractionAt = 0;
  bool preSleepyTriggered = false;
  bool sleepDisplayPoweredOff = false;
  bool soundLookLeft = true;
  bool bluetoothConnected = false;
  bool displayInfoMode = false;
  String chronosTime = "--:--";
  String chronosDate = "-- -- ----";
  String chronosDay = "---";
  String chronosWeather = "Weather: --";
  String chronosTemperature = "";
  String chronosLocation = "";
  bool chronosClockSynced = false;
  unsigned long chronosClockSyncedAt = 0;
  int chronosClockHour24 = 0;
  int chronosClockMinute = 0;
  int chronosClockYear = 2026;
  int chronosClockMonth = 1;
  int chronosClockDay = 1;
  bool chronosClockHasDate = false;
  unsigned long nextChronosTickAt = 0;
  uint8_t infoPage = 0;
  unsigned long nextInfoPageAt = 0;
  unsigned long weatherWindowEndsAt = 0;
  bool demoSequenceActive = false;
  uint8_t demoSequenceStep = 0;
  bool wakeFromSound = false;

  AnimationEngine* animator = nullptr;
  DisplayManager* display = nullptr;
  InputHandler* input = nullptr;

  static const Emotion demoSequence[];
  static const uint8_t demoSequenceCount = 6;

  void logTransition(DeviceState from, DeviceState to) {
    const char* names[] = {"IDLE", "INTERACT", "SLEEP", "WAKE", "EMOTION"};
    Serial.print("State transition: ");
    Serial.print(names[from]);
    Serial.print(" -> ");
    Serial.println(names[to]);
  }

  Emotion randomDifferentEmotion() {
    static const Emotion pool[] = {HAPPY, SLEEPY, SURPRISED, CONFUSED, BLINK};
    const int count = sizeof(pool) / sizeof(pool[0]);

    Emotion selected = pool[random(count)];
    int guard = 0;
    while ((selected == lastRandomEmotion || selected == animator->getEmotion()) && guard < 12) {
      selected = pool[random(count)];
      guard++;
    }

    lastRandomEmotion = selected;
    return selected;
  }

  void markInteraction(unsigned long now) {
    lastInteractionAt = now;
    preSleepyTriggered = false;
  }

  void setInfoModeInternal(bool enabled) {
    displayInfoMode = enabled;
    redrawRequested = true;
  }

  void startDemoSequence(Emotion firstEmotion = SURPRISED) {
    demoSequenceActive = true;
    demoSequenceStep = 0;
    overrideActive = true;
    overrideEmotion = firstEmotion;
    previousState = currentState;
    currentState = STATE_EMOTION_OVERRIDE;
    stateStartedAt = millis();
    displayInfoMode = false;
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_NORMAL);
    display->setPowerSave(false);
    animator->setEmotion(firstEmotion, true);
    animator->setAttention(LOOK_CENTER);
    logTransition(previousState, currentState);
    redrawRequested = true;
  }

  void handleTouchAction(TouchSide side, TouchEvent event, unsigned long now) {
    if (side == TOUCH_SIDE_NONE || event == TOUCH_EVENT_NONE) {
      return;
    }

    markInteraction(now);

    if (event == TOUCH_EVENT_LONG) {
      setInfoMode(false);
      triggerEmotion(HAPPY);
      return;
    }

    if (event == TOUCH_EVENT_DOUBLE) {
      setInfoMode(false);
      startDemoSequence(SURPRISED);
      return;
    }

    if (side == TOUCH_SIDE_LEFT) {
      setInfoMode(true);
      return;
    }

    if (side == TOUCH_SIDE_RIGHT) {
      setInfoMode(false);
      if (currentState == STATE_SLEEP) {
        wakeUp();
      } else {
        startDemoSequence(SURPRISED);
      }
    }
  }

  static String weekdayFromDate(const String& dateValue) {
    int month = 0;
    int day = 0;
    int year = 0;

    if (sscanf(dateValue.c_str(), "%d/%d/%d", &month, &day, &year) != 3) {
      return "---";
    }

    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) {
      return "---";
    }

    static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    if (month < 3) {
      y -= 1;
    }

    int weekday = (y + y / 4 - y / 100 + y / 400 + offsets[month - 1] + day) % 7;
    static const char* names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return String(names[weekday]);
  }

  static bool parseChronosTimeParts(const String& timeValue, int& hour24, int& minute) {
    int colon = timeValue.indexOf(':');
    if (colon < 0) {
      return false;
    }

    hour24 = timeValue.substring(0, colon).toInt();
    minute = timeValue.substring(colon + 1).toInt();
    return hour24 >= 0 && hour24 <= 23 && minute >= 0 && minute <= 59;
  }

  static bool parseChronosDateParts(const String& dateValue, int& month, int& day, int& year) {
    return sscanf(dateValue.c_str(), "%d/%d/%d", &month, &day, &year) == 3 &&
      month >= 1 && month <= 12 && day >= 1 && day <= 31 && year >= 2000 && year <= 2099;
  }

  static bool isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
  }

  static int daysInMonth(int year, int month) {
    static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
      return 29;
    }
    return daysPerMonth[month - 1];
  }

  static void advanceChronosDate(int& year, int& month, int& day, unsigned long daysToAdd) {
    while (daysToAdd > 0) {
      int dim = daysInMonth(year, month);
      day++;
      if (day > dim) {
        day = 1;
        month++;
        if (month > 12) {
          month = 1;
          year++;
        }
      }
      daysToAdd--;
    }
  }

  void syncChronosClockFromPacket() {
    int hour24 = -1;
    int minute = -1;
    if (!parseChronosTimeParts(chronosTime, hour24, minute)) {
      chronosClockSynced = false;
      return;
    }

    chronosClockHour24 = hour24;
    chronosClockMinute = minute;
    chronosClockSyncedAt = millis();
    nextChronosTickAt = chronosClockSyncedAt + 1000UL;
    chronosClockSynced = true;

    int month = 0;
    int day = 0;
    int year = 0;
    if (parseChronosDateParts(chronosDate, month, day, year)) {
      chronosClockMonth = month;
      chronosClockDay = day;
      chronosClockYear = year;
      chronosClockHasDate = true;
      chronosDay = weekdayFromDate(chronosDate);
    }
  }

  void updateChronosClock(unsigned long now) {
    if (!chronosClockSynced || now < nextChronosTickAt) {
      return;
    }

    unsigned long elapsedSeconds = (now - chronosClockSyncedAt) / 1000UL;
    unsigned long totalMinutes = ((unsigned long)chronosClockHour24 * 60UL) + (unsigned long)chronosClockMinute + (elapsedSeconds / 60UL);
    unsigned long dayAdvance = totalMinutes / (24UL * 60UL);
    unsigned long minuteOfDay = totalMinutes % (24UL * 60UL);

    chronosClockHour24 = (int)(minuteOfDay / 60UL);
    chronosClockMinute = (int)(minuteOfDay % 60UL);

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", chronosClockHour24, chronosClockMinute);
    chronosTime = String(buf);

    if (chronosClockHasDate && dayAdvance > 0) {
      advanceChronosDate(chronosClockYear, chronosClockMonth, chronosClockDay, dayAdvance);
      char dateBuf[16];
      snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", chronosClockMonth, chronosClockDay, chronosClockYear);
      chronosDate = String(dateBuf);
      chronosDay = weekdayFromDate(chronosDate);
    }

    nextChronosTickAt = chronosClockSyncedAt + ((elapsedSeconds + 1UL) * 1000UL);
    redrawRequested = true;
  }

  void enterIdle(bool preserveEmotion = false) {
    currentState = STATE_IDLE;
    stateStartedAt = millis();
    overrideActive = false;
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_NORMAL);
    if (!preserveEmotion || animator->getEmotion() != IDLE_NEUTRAL) {
      animator->setEmotion(IDLE_NEUTRAL, true);
    }
    animator->setAttention(LOOK_CENTER);
    nextIdlePulseAt = millis() + random(3800, 8500);
    nextCuriosityAt = millis() + random(3500, 9000);
    nextEmotionShiftAt = millis() + random(16000, 30000);
    redrawRequested = true;
  }

  void enterSleep() {
    previousState = currentState;
    currentState = STATE_SLEEP;
    stateStartedAt = millis();
    setInfoMode(true);
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_DIM);
    display->setPowerSave(false);
    animator->setEmotion(SLEEPY, true);
    animator->setAttention(LOOK_CENTER);
    sleepDisplayPoweredOff = false;
    logTransition(previousState, currentState);
    redrawRequested = true;
  }

  void enterWake() {
    previousState = currentState;
    currentState = STATE_WAKE;
    stateStartedAt = millis();
    setInfoMode(false);
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_NORMAL);
    display->setPowerSave(false);
    animator->setEmotion(SURPRISED, true);
    animator->setAttention(LOOK_CENTER);
    sleepDisplayPoweredOff = false;
    logTransition(previousState, currentState);
    redrawRequested = true;
  }

  void enterInteract() {
    previousState = currentState;
    currentState = STATE_INTERACT;
    stateStartedAt = millis();
    setInfoMode(false);
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_NORMAL);
    display->setPowerSave(false);
    animator->setEmotion(HAPPY, true);
    animator->setAttention(LOOK_CENTER);
    logTransition(previousState, currentState);
    redrawRequested = true;
  }

  void enterEmotion(Emotion emotion) {
    previousState = currentState;
    currentState = STATE_EMOTION_OVERRIDE;
    overrideEmotion = emotion;
    overrideActive = true;
    stateStartedAt = millis();
    setInfoMode(false);
    display->setSleepMode(false);
    display->setBrightness(BRIGHTNESS_NORMAL);
    display->setPowerSave(false);
    animator->setEmotion(emotion, true);
    animator->setAttention(LOOK_CENTER);
    logTransition(previousState, currentState);
    redrawRequested = true;
  }

public:
  StateMachine(AnimationEngine* anim, DisplayManager* disp, InputHandler* inp)
    : animator(anim), display(disp), input(inp) {
    stateStartedAt = millis();
    lastInteractionAt = stateStartedAt;
    nextIdlePulseAt = millis() + 1000;
    nextCuriosityAt = millis() + 2500;
  }

  void begin() {
    lastInteractionAt = millis();
    preSleepyTriggered = false;
    enterIdle(false);
  }

  DeviceState getState() const {
    return currentState;
  }

  const char* getStateName() const {
    switch (currentState) {
      case STATE_INTERACT: return "interact";
      case STATE_SLEEP: return "sleep";
      case STATE_WAKE: return "wake";
      case STATE_EMOTION_OVERRIDE: return "emotion";
      case STATE_IDLE:
      default: return "idle";
    }
  }

  const char* getEmotionName() const {
    return animator->getEmotionName();
  }

  bool isBluetoothConnected() const {
    return bluetoothConnected;
  }

  bool isInfoMode() const {
    return displayInfoMode;
  }

  void setInfoMode(bool enabled) {
    displayInfoMode = enabled;
    if (enabled) {
      infoPage = 0;
      nextInfoPageAt = millis() + 180000UL;
      weatherWindowEndsAt = 0;
    }
    redrawRequested = true;
  }

  void setBluetoothConnected(bool connected) {
    bluetoothConnected = connected;
    if (connected) {
      infoPage = 0;
      nextInfoPageAt = millis() + 180000UL;
      weatherWindowEndsAt = 0;
    } else {
      displayInfoMode = false;
      weatherWindowEndsAt = 0;
    }
    redrawRequested = true;
  }

  void toggleInfoMode() {
    if (!bluetoothConnected) {
      return;
    }

    setInfoMode(!displayInfoMode);
    display->recordActivity();
  }

  void setChronosInfo(const String& timeValue, const String& dateValue, const String& weatherValue, const String& tempValue = "", const String& locationValue = "") {
    if (timeValue.length() > 0) {
      chronosTime = timeValue;
    }
    if (dateValue.length() > 0) {
      chronosDate = dateValue;
      chronosDay = weekdayFromDate(dateValue);
    }
    if (weatherValue.length() > 0) {
      chronosWeather = weatherValue;
    }
    if (tempValue.length() > 0) {
      chronosTemperature = tempValue;
    }
    if (locationValue.length() > 0) {
      chronosLocation = locationValue;
    }
    if (timeValue.length() > 0) {
      syncChronosClockFromPacket();
    }
    redrawRequested = true;
  }

  void setBrightness(int brightness) {
    display->setBrightness(brightness);
  }

  void triggerEmotion(Emotion emotion) {
    lastRandomEmotion = emotion;
    enterEmotion(emotion);
    display->recordActivity();
  }

  void wakeUp() {
    enterWake();
    display->recordActivity();
  }

  void sleepNow() {
    enterSleep();
  }

  void returnToIdle() {
    enterIdle(false);
  }

  void setEmotionByName(const String& name) {
    String lowered = name;
    lowered.toLowerCase();

    if (lowered == "happy" || lowered == "love" || lowered == "love you") {
      triggerEmotion(HAPPY);
    } else if (lowered == "sleepy" || lowered == "sleep") {
      triggerEmotion(SLEEPY);
    } else if (lowered == "angry") {
      triggerEmotion(CONFUSED);
    } else if (lowered == "game") {
      triggerEmotion(CONFUSED);
    } else if (lowered == "surprised") {
      triggerEmotion(SURPRISED);
    } else if (lowered == "blink") {
      triggerEmotion(BLINK);
    } else if (lowered == "confused") {
      triggerEmotion(CONFUSED);
    } else {
      returnToIdle();
    }
  }

  bool handleVoiceCommand(String spokenText) {
    markInteraction(millis());
    spokenText.trim();
    spokenText.toLowerCase();
    if (spokenText.length() == 0) {
      return false;
    }

    if (spokenText.startsWith("modo ")) {
      spokenText.remove(0, 5);
    } else if (spokenText.startsWith("mojo ")) {
      spokenText.remove(0, 5);
    } else if (spokenText.startsWith("mochi ")) {
      spokenText.remove(0, 6);
    }

    spokenText.trim();
    if (spokenText.length() == 0) {
      return false;
    }

    if (spokenText.indexOf("sleep") >= 0) {
      sleepNow();
      return true;
    }

    if (spokenText.indexOf("angry") >= 0) {
      triggerEmotion(CONFUSED);
      return true;
    }

    if (spokenText.indexOf("game") >= 0) {
      triggerEmotion(CONFUSED);
      return true;
    }

    if (spokenText.indexOf("love") >= 0) {
      triggerEmotion(HAPPY);
      return true;
    }

    if (spokenText.indexOf("wake") >= 0) {
      wakeUp();
      return true;
    }

    if (spokenText.indexOf("happy") >= 0) {
      triggerEmotion(HAPPY);
      return true;
    }

    if (spokenText.indexOf("blink") >= 0) {
      triggerEmotion(BLINK);
      return true;
    }

    if (spokenText.indexOf("surprise") >= 0) {
      triggerEmotion(SURPRISED);
      return true;
    }

    if (spokenText.indexOf("confuse") >= 0) {
      triggerEmotion(CONFUSED);
      return true;
    }

    return false;
  }

  String statusJson() const {
    String json = "{";
    json += "\"state\":\"" + String(getStateName()) + "\",";
    json += "\"emotion\":\"" + String(getEmotionName()) + "\",";
    json += "\"bluetooth\":" + String(bluetoothConnected ? "true" : "false") + ",";
    json += "\"info_mode\":" + String(displayInfoMode ? "true" : "false") + ",";
    json += "\"time\":\"" + chronosTime + "\",";
    json += "\"date\":\"" + chronosDate + "\",";
    json += "\"day\":\"" + chronosDay + "\",";
    json += "\"weather\":\"" + chronosWeather + "\",";
    json += "\"brightness\":" + String(display->getBrightness()) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += "}";
    return json;
  }

  void update() {
    input->update();
    ButtonEvent event = input->consumeEvent();
    TouchSide touchSide = input->consumeTouchSide();
    TouchEvent touchEvent = input->consumeTouchEvent();
    unsigned long now = millis();

    updateChronosClock(now);

    if (event != BUTTON_NONE || touchSide != TOUCH_SIDE_NONE || touchEvent != TOUCH_EVENT_NONE || input->isButtonPressed() || input->isTouchActive()) {
      markInteraction(now);
    }

    if (touchEvent != TOUCH_EVENT_NONE) {
      handleTouchAction(touchSide, touchEvent, now);
      return;
    }

    if (bluetoothConnected && touchSide == TOUCH_SIDE_LEFT && currentState != STATE_SLEEP) {
      toggleInfoMode();
      return;
    }

    if (bluetoothConnected && touchSide == TOUCH_SIDE_RIGHT && currentState != STATE_SLEEP) {
      setInfoMode(false);
      if (currentState == STATE_IDLE) {
        enterEmotion(randomDifferentEmotion());
      }
      return;
    }

    switch (currentState) {
      case STATE_IDLE:
#if ENABLE_SOUND_REACTIVE
        if (input->detectSound(AUDIO_THRESHOLD)) {
#if ENABLE_SERIAL_DEBUG
          Serial.print("Mic detected sound: level=");
          Serial.print(input->getSensorValue());
          Serial.print(" delta=");
          Serial.println(input->getSensorDelta());
#endif
          setInfoMode(false);
          markInteraction(now);
          wakeFromSound = false;
          animator->setAttention(soundLookLeft ? LOOK_LEFT : LOOK_RIGHT);
          soundLookLeft = !soundLookLeft;
          startDemoSequence(SURPRISED);
          animator->setAttention(soundLookLeft ? LOOK_LEFT : LOOK_RIGHT);
          redrawRequested = true;
        }
#endif

        if (displayInfoMode && bluetoothConnected) {
          if (weatherWindowEndsAt > 0) {
            if (now >= weatherWindowEndsAt) {
              infoPage = 0;
              weatherWindowEndsAt = 0;
              nextInfoPageAt = now + 180000UL;
              redrawRequested = true;
            }
          } else if (now >= nextInfoPageAt) {
            infoPage = 1;
            weatherWindowEndsAt = now + 60000UL;
            redrawRequested = true;
          }
        }

        if ((now - lastInteractionAt) >= SENSOR_IDLE_SLEEP_TIMEOUT) {
          enterSleep();
          break;
        }

        if (!preSleepyTriggered && (now - lastInteractionAt) >= PRE_SLEEPY_TIMEOUT) {
          preSleepyTriggered = true;
          enterEmotion(CONFUSED);
          break;
        }

        if (event == BUTTON_LONG_PRESS) {
          enterSleep();
          break;
        }

        if (event == BUTTON_DOUBLE_PRESS) {
          enterEmotion(CONFUSED);
          break;
        }

        if (!bluetoothConnected && touchSide == TOUCH_SIDE_LEFT) {
          animator->setAttention(LOOK_LEFT);
          enterEmotion(randomDifferentEmotion());
          break;
        }

        if (!bluetoothConnected && touchSide == TOUCH_SIDE_RIGHT) {
          animator->setAttention(LOOK_RIGHT);
          enterEmotion(randomDifferentEmotion());
          break;
        }

        if (event == BUTTON_SHORT_PRESS) {
          enterEmotion(randomDifferentEmotion());
          break;
        }

        if (now >= nextIdlePulseAt) {
          animator->randomMoodDrift();
          nextIdlePulseAt = now + random(3800, 8500);
        }

        if (now >= nextCuriosityAt) {
          animator->setAttention((random(2) == 0) ? LOOK_LEFT : LOOK_RIGHT);
          nextCuriosityAt = now + random(3500, 9000);
        }

        if (now >= nextEmotionShiftAt) {
          enterEmotion(randomDifferentEmotion());
          nextEmotionShiftAt = now + random(18000, 32000);
        }
        break;

      case STATE_INTERACT:
        if (event == BUTTON_LONG_PRESS) {
          enterSleep();
          break;
        }

        if (touchSide == TOUCH_SIDE_LEFT) {
          animator->triggerBlink(false);
          animator->setAttention(LOOK_LEFT);
        } else if (touchSide == TOUCH_SIDE_RIGHT) {
          animator->triggerBlink(false);
          animator->setAttention(LOOK_RIGHT);
        } else if (event == BUTTON_SHORT_PRESS) {
          animator->triggerBlink(false);
          animator->setAttention((random(2) == 0) ? LOOK_LEFT : LOOK_RIGHT);
        }

        if ((now - stateStartedAt) >= INTERACT_DURATION && animator->consumeCycleCompleted()) {
          enterIdle(true);
        }
        break;

      case STATE_SLEEP:
        if (input->detectSound(AUDIO_THRESHOLD)) {
          markInteraction(now);
          wakeFromSound = true;
          setInfoMode(false);
          animator->setAttention(soundLookLeft ? LOOK_LEFT : LOOK_RIGHT);
          soundLookLeft = !soundLookLeft;
          startDemoSequence(SURPRISED);
          animator->setAttention(soundLookLeft ? LOOK_LEFT : LOOK_RIGHT);
          break;
        }

        if (event == BUTTON_SHORT_PRESS || event == BUTTON_DOUBLE_PRESS || input->isButtonPressed() || touchSide != TOUCH_SIDE_NONE) {
          markInteraction(now);
          wakeFromSound = false;
          setInfoMode(false);
          enterWake();
          break;
        }

        if (!sleepDisplayPoweredOff && animator->consumeCycleCompleted()) {
          display->setPowerSave(true);
          sleepDisplayPoweredOff = true;
        }
        break;

      case STATE_WAKE:
        if (animator->consumeCycleCompleted()) {
          if (wakeFromSound) {
            wakeFromSound = false;
            enterSleep();
          } else {
            enterIdle(false);
          }
        }
        break;

      case STATE_EMOTION_OVERRIDE:
        if (event == BUTTON_LONG_PRESS) {
          enterSleep();
          break;
        }

        if (animator->consumeCycleCompleted()) {
          if (demoSequenceActive) {
            demoSequenceStep++;
            if (demoSequenceStep < demoSequenceCount) {
              animator->setEmotion(demoSequence[demoSequenceStep], true);
              animator->setAttention((demoSequenceStep % 2 == 0) ? LOOK_LEFT : LOOK_RIGHT);
              redrawRequested = true;
            } else {
              demoSequenceActive = false;
              enterIdle(false);
            }
          } else {
            enterIdle(false);
          }
        }
        break;
    }

    frameAdvancedThisTick = animator->update();
  }

  void render() {
    if (displayInfoMode && bluetoothConnected) {
      if (infoPage == 0) {
        display->showChronosClock(chronosTime, chronosDate, chronosDay);
      } else if (infoPage == 1) {
        display->showChronosWeather(chronosLocation, chronosTemperature, chronosWeather);
      }
      frameAdvancedThisTick = false;
      redrawRequested = false;
      return;
    }

    if (!frameAdvancedThisTick && !redrawRequested) {
      return;
    }

    int frameIndex = animator->getCurrentFrame();
    frameIndex = constrain(frameIndex, 0, TOTAL_FRAMES - 1);
    display->drawFrame(frames[frameIndex], FRAME_WIDTH, FRAME_HEIGHT);
    redrawRequested = false;
    frameAdvancedThisTick = false;
  }
};

const Emotion StateMachine::demoSequence[] = {
  SURPRISED,
  HAPPY,
  CONFUSED,
  BLINK,
  SLEEPY,
  IDLE_NEUTRAL
};

#endif // STATE_MACHINE_H
