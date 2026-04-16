#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Wire.h>
#include <U8g2lib.h>
#include <pgmspace.h>
#include "config.h"

#if DISPLAY_IS_SH1106
using MochiDisplay = U8G2_SH1106_128X64_NONAME_F_HW_I2C;
#else
using MochiDisplay = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
#endif

class DisplayManager {
private:
  MochiDisplay display;
  int currentBrightness = BRIGHTNESS_NORMAL;
  bool initialized = false;
  unsigned long lastActivityTime = 0;
  uint8_t renderBuffer[(SCREEN_WIDTH * SCREEN_HEIGHT) / 8];

  static bool parseClockTime(const String& timeValue, int& hour24, int& minute) {
    hour24 = -1;
    minute = -1;

    int colon = timeValue.indexOf(':');
    if (colon < 0) {
      return false;
    }

    hour24 = timeValue.substring(0, colon).toInt();
    minute = timeValue.substring(colon + 1).toInt();
    return hour24 >= 0 && hour24 <= 23 && minute >= 0 && minute <= 59;
  }

  static String formatClockDisplay(const String& timeValue, String& periodOut) {
    periodOut = "";

    int hour24 = -1;
    int minute = -1;
    if (!parseClockTime(timeValue, hour24, minute)) {
      return timeValue.length() > 0 ? timeValue : "--:--";
    }

    periodOut = (hour24 >= 12) ? "PM" : "AM";
    int hour12 = hour24 % 12;
    if (hour12 == 0) {
      hour12 = 12;
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour12, minute);
    return String(buf);
  }

  static String formatDateDisplay(const String& dateValue, const String& dayValue) {
    int month = 0;
    int day = 0;
    int year = 0;

    if (sscanf(dateValue.c_str(), "%d/%d/%d", &month, &day, &year) == 3) {
      static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
      if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        String prefix = dayValue;
        if (prefix.length() == 0) {
          prefix = "---";
        }

        char buf[24];
        snprintf(buf, sizeof(buf), "%s, %s %02d", prefix.c_str(), months[month - 1], day);
        return String(buf);
      }
    }

    if (dateValue.length() > 0 && dayValue.length() > 0) {
      return dayValue + ", " + dateValue;
    }

    if (dateValue.length() > 0) {
      return dateValue;
    }

    return dayValue.length() > 0 ? dayValue : "";
  }

  void drawWeatherIcon(const String& condition, int x, int y) {
    String lowered = condition;
    lowered.toLowerCase();

    bool rainy = lowered.indexOf("rain") >= 0 || lowered.indexOf("drizzle") >= 0 || lowered.indexOf("shower") >= 0;
    bool cloudy = lowered.indexOf("cloud") >= 0 || lowered.indexOf("overcast") >= 0;
    bool storm = lowered.indexOf("thunder") >= 0 || lowered.indexOf("storm") >= 0;
    bool foggy = lowered.indexOf("fog") >= 0 || lowered.indexOf("mist") >= 0 || lowered.indexOf("haze") >= 0;
    bool sunny = lowered.indexOf("sun") >= 0 || lowered.indexOf("clear") >= 0;

    if (storm) {
      display.drawCircle(x + 12, y + 12, 9);
      display.drawBox(x + 5, y + 13, 18, 8);
      display.drawLine(x + 12, y + 18, x + 9, y + 28);
      display.drawLine(x + 9, y + 28, x + 15, y + 22);
      display.drawLine(x + 15, y + 22, x + 13, y + 32);
      return;
    }

    if (rainy) {
      display.drawCircle(x + 10, y + 10, 7);
      display.drawBox(x + 4, y + 12, 20, 8);
      display.drawLine(x + 8, y + 23, x + 6, y + 29);
      display.drawLine(x + 14, y + 23, x + 12, y + 29);
      display.drawLine(x + 20, y + 23, x + 18, y + 29);
      return;
    }

    if (cloudy) {
      display.drawCircle(x + 9, y + 13, 7);
      display.drawCircle(x + 16, y + 11, 8);
      display.drawCircle(x + 23, y + 14, 6);
      display.drawBox(x + 7, y + 16, 20, 8);
      return;
    }

    if (foggy) {
      display.drawLine(x + 4, y + 12, x + 27, y + 12);
      display.drawLine(x + 2, y + 18, x + 29, y + 18);
      display.drawLine(x + 4, y + 24, x + 27, y + 24);
      return;
    }

    if (sunny) {
      display.drawCircle(x + 14, y + 14, 6);
      display.drawLine(x + 14, y + 2, x + 14, y + 7);
      display.drawLine(x + 14, y + 21, x + 14, y + 26);
      display.drawLine(x + 2, y + 14, x + 7, y + 14);
      display.drawLine(x + 21, y + 14, x + 26, y + 14);
      display.drawLine(x + 6, y + 6, x + 9, y + 9);
      display.drawLine(x + 19, y + 19, x + 22, y + 22);
      display.drawLine(x + 19, y + 9, x + 22, y + 6);
      display.drawLine(x + 6, y + 22, x + 9, y + 19);
      return;
    }

    display.drawCircle(x + 14, y + 14, 7);
    display.drawLine(x + 14, y + 7, x + 14, y + 21);
    display.drawLine(x + 7, y + 14, x + 21, y + 14);
  }

public:
  DisplayManager() : display(U8G2_R0) {}

  bool begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(I2C_CLOCK);
    delay(50);

    display.begin();
    display.setContrast(BRIGHTNESS_NORMAL);
    display.clearBuffer();
    display.setFont(u8g2_font_profont11_tf);
    display.drawStr(18, 22, "Dasai Mochi");
    display.drawStr(22, 40, "Waking up...");
    display.sendBuffer();

    initialized = true;
    recordActivity();
    return true;
  }

  void drawFrame(const uint8_t* frameData, int width, int height) {
    if (!initialized) {
      return;
    }

    display.clearBuffer();
#if FRAME_DATA_IS_XBM
    display.drawXBMP(0, 0, width, height, frameData);
#else
  size_t bytesToCopy = (size_t)((width * height) / 8);
  if (bytesToCopy > sizeof(renderBuffer)) {
    bytesToCopy = sizeof(renderBuffer);
  }
  memcpy_P(renderBuffer, frameData, bytesToCopy);
  display.drawBitmap(0, 0, (width + 7) / 8, height, renderBuffer);
#endif
    display.sendBuffer();
    recordActivity();
  }

  void drawStatusFrame(const uint8_t* frameData, int width, int height, const char* label) {
    if (!initialized) {
      return;
    }

    display.clearBuffer();
  #if FRAME_DATA_IS_XBM
    display.drawXBMP(0, 0, width, height, frameData);
  #else
    size_t bytesToCopy = (size_t)((width * height) / 8);
    if (bytesToCopy > sizeof(renderBuffer)) {
      bytesToCopy = sizeof(renderBuffer);
    }
    memcpy_P(renderBuffer, frameData, bytesToCopy);
    display.drawBitmap(0, 0, (width + 7) / 8, height, renderBuffer);
  #endif
    display.setFont(u8g2_font_5x8_tf);
    display.drawStr(2, 8, label);
    display.sendBuffer();
    recordActivity();
  }

  void setBrightness(int brightness) {
    currentBrightness = constrain(brightness, 0, 255);
    display.setContrast(currentBrightness);
  }

  int getBrightness() const {
    return currentBrightness;
  }

  void setPowerSave(bool enabled) {
    display.setPowerSave(enabled ? 1 : 0);
  }

  void setSleepMode(bool sleep) {
    if (sleep) {
      setBrightness(BRIGHTNESS_SLEEP);
      setPowerSave(true);
    } else {
      setPowerSave(false);
      setBrightness(BRIGHTNESS_NORMAL);
    }
  }

  void recordActivity() {
    lastActivityTime = millis();
  }

  unsigned long getInactivityTime() const {
    return millis() - lastActivityTime;
  }

  void clearDisplay() {
    if (!initialized) {
      return;
    }

    display.clearBuffer();
    display.sendBuffer();
  }

  void showSplash(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
    if (!initialized) {
      return;
    }

    display.clearBuffer();
    display.setFont(u8g2_font_profont11_tf);
    display.drawStr(6, 20, line1);

    if (line2 != nullptr && line2[0] != '\0') {
      display.drawStr(6, 35, line2);
    }

    if (line3 != nullptr && line3[0] != '\0') {
      display.drawStr(6, 50, line3);
    }

    display.sendBuffer();
  }

  void showChronosClock(const String& timeValue, const String& dateValue, const String& dayValue) {
    if (!initialized) {
      return;
    }

    String formattedTime;
    String period;
    formattedTime = formatClockDisplay(timeValue, period);
    String formattedDate = formatDateDisplay(dateValue, dayValue);

    display.clearBuffer();

    display.setFont(u8g2_font_logisoso24_tn);
    int16_t timeWidth = display.getStrWidth(formattedTime.c_str());
    int16_t timeX = (SCREEN_WIDTH - timeWidth) / 2;
    if (timeX < 0) {
      timeX = 0;
    }
    display.drawStr(timeX, 30, formattedTime.c_str());

    if (period.length() > 0) {
      display.setFont(u8g2_font_6x12_tf);
      display.drawStr(104, 12, period.c_str());
    }

    if (formattedDate.length() > 0) {
      display.setFont(u8g2_font_6x12_tf);
      int16_t dateWidth = display.getStrWidth(formattedDate.c_str());
      int16_t dateX = (SCREEN_WIDTH - dateWidth) / 2;
      if (dateX < 0) {
        dateX = 0;
      }
      display.drawStr(dateX, 57, formattedDate.c_str());
    }

    display.sendBuffer();
  }

  void showChronosWeather(const String& locationValue, const String& temperatureValue, const String& conditionValue) {
    if (!initialized) {
      return;
    }

    String location = locationValue.length() > 0 ? locationValue : "--";
    location.toUpperCase();
    String temperature = temperatureValue.length() > 0 ? temperatureValue : "--°C";
    String condition = conditionValue.length() > 0 ? conditionValue : "Weather: --";

    display.clearBuffer();

    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(0, 13, location.c_str());

    display.setFont(u8g2_font_logisoso24_tn);
    display.drawStr(0, 42, temperature.c_str());

    drawWeatherIcon(condition, 91, 4);

    display.drawHLine(0, 45, 88);

    display.setFont(u8g2_font_5x8_tf);
    display.drawStr(0, 61, condition.c_str());

    display.sendBuffer();
  }

  MochiDisplay& raw() {
    return display;
  }
};

#endif // DISPLAY_MANAGER_H
