#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "state_machine.h"

#if ENABLE_WEATHER_API

class WeatherService {
private:
  unsigned long nextUpdateAt = 0;
  bool firstFetchDone = false;
  String lastCondition;
  String lastTemperature;
  String lastLocation;

  static String trimString(String value) {
    value.trim();
    while (value.startsWith("\"") || value.startsWith("'")) {
      value.remove(0, 1);
      value.trim();
    }
    while (value.endsWith("\"") || value.endsWith("'")) {
      value.remove(value.length() - 1, 1);
      value.trim();
    }
    return value;
  }

  static String extractJsonString(const String& source, const char* key) {
    String pattern = String(key);
    int startIndex = source.indexOf(pattern);
    if (startIndex < 0) {
      return "";
    }

    startIndex += pattern.length();
    int endIndex = source.indexOf('"', startIndex);
    if (endIndex < 0 || endIndex <= startIndex) {
      return "";
    }

    return trimString(source.substring(startIndex, endIndex));
  }

  static String extractJsonNumber(const String& source, const char* key) {
    String pattern = String(key);
    int startIndex = source.indexOf(pattern);
    if (startIndex < 0) {
      return "";
    }

    startIndex += pattern.length();
    int endIndex = startIndex;
    while (endIndex < (int)source.length()) {
      char c = source[endIndex];
      if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
        endIndex++;
      } else {
        break;
      }
    }

    if (endIndex <= startIndex) {
      return "";
    }

    return trimString(source.substring(startIndex, endIndex));
  }

  static String formatTemperature(const String& rawTemp) {
    if (rawTemp.length() == 0) {
      return "";
    }

    float temp = rawTemp.toFloat();
    if (temp < -99.0f || temp > 99.0f) {
      return rawTemp + "°";
    }

    char buf[16];
    if (fabs(temp - roundf(temp)) < 0.05f) {
      snprintf(buf, sizeof(buf), "%d°C", (int)roundf(temp));
    } else {
      snprintf(buf, sizeof(buf), "%.1f°C", temp);
    }
    return String(buf);
  }

  bool connectWiFi() {
    if (String(WEATHER_WIFI_SSID).length() == 0 || String(WEATHER_WIFI_PASSWORD).length() == 0) {
#if ENABLE_SERIAL_DEBUG
      Serial.println("Weather WiFi not configured (set WEATHER_WIFI_SSID / WEATHER_WIFI_PASSWORD)");
#endif
      return false;
    }

    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.print("Connecting WiFi for weather: ");
    Serial.println(WEATHER_WIFI_SSID);
#endif
    WiFi.mode(WIFI_STA);
    WiFi.begin(WEATHER_WIFI_SSID, WEATHER_WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000UL) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
#if ENABLE_SERIAL_DEBUG
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());
#endif
      return true;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.println("WiFi connect failed for weather");
#endif
    return false;
  }

  void disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(true, true);
    }
    WiFi.mode(WIFI_OFF);
  }

  bool fetchWeather(StateMachine* stateMachine) {
    if (stateMachine == nullptr) {
      return false;
    }

    if (String(OPENWEATHER_API_KEY).length() == 0) {
#if ENABLE_SERIAL_DEBUG
      Serial.println("OpenWeather API key is missing");
#endif
      return false;
    }

    if (!connectWiFi()) {
      return false;
    }

    String url = "http://api.openweathermap.org/data/2.5/weather?appid=";
    url += OPENWEATHER_API_KEY;
    url += "&units=metric";

#if WEATHER_USE_COORDINATES
    if (String(WEATHER_LATITUDE).length() > 0 && String(WEATHER_LONGITUDE).length() > 0) {
      url += "&lat=";
      url += WEATHER_LATITUDE;
      url += "&lon=";
      url += WEATHER_LONGITUDE;
    } else
#endif
    {
      url += "&q=";
      url += WEATHER_QUERY;
    }

    HTTPClient http;
    http.setTimeout(12000);
    if (!http.begin(url)) {
#if ENABLE_SERIAL_DEBUG
      Serial.println("Weather HTTP begin failed");
#endif
      disconnectWiFi();
      return false;
    }

    int httpCode = http.GET();
    if (httpCode <= 0) {
#if ENABLE_SERIAL_DEBUG
      Serial.print("Weather HTTP error: ");
      Serial.println(httpCode);
#endif
      http.end();
      disconnectWiFi();
      return false;
    }

    String payload = http.getString();
    http.end();
    disconnectWiFi();

    if (payload.length() == 0) {
      return false;
    }

    String location = extractJsonString(payload, "\"name\":\"");
    String condition = extractJsonString(payload, "\"description\":\"");
    if (condition.length() == 0) {
      condition = extractJsonString(payload, "\"main\":\"");
    }
    String temp = extractJsonNumber(payload, "\"temp\":");
    String tempLine = formatTemperature(temp);

    if (location.length() == 0) {
      location = WEATHER_QUERY;
    }
    if (condition.length() == 0) {
      condition = "Weather unavailable";
    }
    if (tempLine.length() == 0) {
      tempLine = "--°C";
    }

    lastLocation = location;
    lastCondition = condition;
    lastTemperature = tempLine;

#if ENABLE_SERIAL_DEBUG
    Serial.print("Weather parsed => location:");
    Serial.print(location);
    Serial.print(" temp:");
    Serial.print(tempLine);
    Serial.print(" condition:");
    Serial.println(condition);
#endif

    stateMachine->setChronosInfo("", "", condition, tempLine, location);
    return true;
  }

public:
  void begin() {
    nextUpdateAt = 0;
    firstFetchDone = false;
  }

  void update(StateMachine* stateMachine) {
    unsigned long now = millis();
    if (!firstFetchDone || now >= nextUpdateAt) {
      bool ok = fetchWeather(stateMachine);
      firstFetchDone = true;
      nextUpdateAt = now + (ok ? WEATHER_UPDATE_INTERVAL : 300000UL);
    }
  }
};

#endif // ENABLE_WEATHER_API
#endif // WEATHER_SERVICE_H
