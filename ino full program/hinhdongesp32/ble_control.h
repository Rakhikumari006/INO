#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include "config.h"

#if defined(ARDUINO_ARCH_ESP32) && ENABLE_BLE_CONTROL

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "state_machine.h"

class BleControl;

class BleServerCallbacks : public BLEServerCallbacks {
private:
  BleControl* owner = nullptr;

public:
  explicit BleServerCallbacks(BleControl* bleControl) : owner(bleControl) {}
  void onConnect(BLEServer* server) override;
  void onDisconnect(BLEServer* server) override;
};

class BleCommandCallbacks : public BLECharacteristicCallbacks {
private:
  BleControl* owner = nullptr;

public:
  explicit BleCommandCallbacks(BleControl* bleControl) : owner(bleControl) {}
  void onWrite(BLECharacteristic* characteristic) override;
};

class BleControl {
private:
  BLEServer* server = nullptr;
  BLEService* service = nullptr;
  BLECharacteristic* statusCharacteristic = nullptr;
  BLECharacteristic* commandCharacteristic = nullptr;
  StateMachine* stateMachine = nullptr;
  bool started = false;
  bool connected = false;
  String lastStatus;
  unsigned long lastStatusNotifyAt = 0;

  static constexpr unsigned long STATUS_NOTIFY_INTERVAL_MS = 5000UL;

  static const char* SERVICE_UUID;
  static const char* STATUS_UUID;
  static const char* COMMAND_UUID;

  static String normalizeCommand(String value) {
    value.trim();
    value.toLowerCase();
    return value;
  }

  static String extractJsonValue(const String& source, const char* key) {
    String pattern = String("\"") + key + "\":";
    int keyIndex = source.indexOf(pattern);
    if (keyIndex < 0) {
      return "";
    }

    int valueStart = keyIndex + pattern.length();
    while (valueStart < (int)source.length() && (source[valueStart] == ' ' || source[valueStart] == '"')) {
      valueStart++;
    }

    int valueEnd = valueStart;
    while (valueEnd < (int)source.length() && source[valueEnd] != '"' && source[valueEnd] != ',' && source[valueEnd] != '}') {
      valueEnd++;
    }

    String value = source.substring(valueStart, valueEnd);
    value.trim();
    return value;
  }

  static String extractKeyValue(const String& source, const char* key) {
    String pattern = String(key) + "=";
    int keyIndex = source.indexOf(pattern);
    if (keyIndex < 0) {
      return "";
    }

    int valueStart = keyIndex + pattern.length();
    int valueEnd = valueStart;
    while (valueEnd < (int)source.length() && source[valueEnd] != ',' && source[valueEnd] != ';' && source[valueEnd] != '\n' && source[valueEnd] != '\r') {
      valueEnd++;
    }

    String value = source.substring(valueStart, valueEnd);
    value.trim();
    return value;
  }

  static String pickFirstNonEmpty(const String& a, const String& b, const String& c = "", const String& d = "") {
    if (a.length() > 0) return a;
    if (b.length() > 0) return b;
    if (c.length() > 0) return c;
    if (d.length() > 0) return d;
    return "";
  }

  static bool isMostlyPrintable(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
      return false;
    }

    int printable = 0;
    for (size_t i = 0; i < len; i++) {
      uint8_t b = data[i];
      if ((b >= 32 && b <= 126) || b == '\n' || b == '\r' || b == '\t') {
        printable++;
      }
    }
    return printable >= (int)(len * 0.6f);
  }

  static String bytesToHex(const uint8_t* data, size_t len) {
    static const char* hex = "0123456789ABCDEF";
    String out;
    out.reserve((int)(len * 3));
    for (size_t i = 0; i < len; i++) {
      uint8_t b = data[i];
      out += hex[(b >> 4) & 0x0F];
      out += hex[b & 0x0F];
      if (i + 1 < len) {
        out += ' ';
      }
    }
    return out;
  }

  static String bytesToAsciiLoose(const uint8_t* data, size_t len) {
    String out;
    out.reserve((int)len);
    for (size_t i = 0; i < len; i++) {
      uint8_t b = data[i];
      if (b >= 32 && b <= 126) {
        out += (char)b;
      } else {
        out += ' ';
      }
    }
    return out;
  }

  static bool decodeBcdTime(const uint8_t* data, size_t len, String& outTime) {
    auto validBcd = [](uint8_t b) {
      return ((b >> 4) <= 9) && ((b & 0x0F) <= 9);
    };

    for (size_t i = 0; i + 1 < len; i++) {
      uint8_t h = data[i];
      uint8_t m = data[i + 1];
      if (!validBcd(h) || !validBcd(m)) {
        continue;
      }

      int hour = ((h >> 4) * 10) + (h & 0x0F);
      int minute = ((m >> 4) * 10) + (m & 0x0F);
      if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
        outTime = String(buf);
        return true;
      }
    }
    return false;
  }

  static bool isAa55Packet(const uint8_t* data, size_t len) {
    return data != nullptr && len >= 4 && data[0] == 0xAA && data[1] == 0x55;
  }

  static String weatherIconToText(uint8_t icon) {
    switch (icon) {
      case 0: return "Clear";
      case 1: return "Partly cloudy";
      case 2: return "Cloudy";
      case 3: return "Rain";
      case 4: return "Heavy rain";
      case 5: return "Storm";
      case 6: return "Snow";
      case 7: return "Fog";
      case 8: return "Drizzle";
      case 9: return "Overcast";
      default:
        if (icon < 20) {
          return String("Weather icon ") + String(icon);
        }
        return "";
    }
  }

  static String formatCelsiusByte(uint8_t value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u°C", value);
    return String(buf);
  }

  void handleAa55TimePacket(const uint8_t* data, size_t len) {
    if (len < 12) {
      return;
    }

    int year = 2000 + (int)data[4];
    int month = (int)data[5];
    int day = (int)data[6];
    int hour = (int)data[7];
    int minute = (int)data[8];

    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      return;
    }

    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour, minute);
    char dateBuf[16];
    snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", month, day, year);

    stateMachine->setChronosInfo(String(timeBuf), String(dateBuf), "");

#if ENABLE_SERIAL_DEBUG
    Serial.print("AA55 time => ");
    Serial.print(timeBuf);
    Serial.print(" date:");
    Serial.println(dateBuf);
#endif
  }

  void handleAa55WeatherPacket(const uint8_t* data, size_t len) {
    if (len < 10) {
      return;
    }

    size_t index = 4;
    uint8_t cityLen = data[index++];
    if (index + cityLen >= len) {
      return;
    }

    String location;
    location.reserve(cityLen);
    for (uint8_t i = 0; i < cityLen && index < len; i++, index++) {
      location += (char)data[index];
    }

    if (index + 7 > len) {
      return;
    }

    uint8_t tempRaw = data[index++];
    uint8_t icon = data[index++];
    uint8_t humidity = data[index++];
    uint16_t pressure = ((uint16_t)data[index] << 8) | (uint16_t)data[index + 1];
    index += 2;
    uint8_t uv = data[index++];
    uint8_t wind = data[index++];

    String condition = weatherIconToText(icon);
    if (condition.length() == 0) {
      condition = "Weather";
    }

    String temperature = formatCelsiusByte(tempRaw);
    if (location.length() == 0) {
      location = "Bhilai";
    }

    stateMachine->setChronosInfo("", "", condition, temperature, location);

#if ENABLE_SERIAL_DEBUG
    Serial.print("AA55 weather => city:");
    Serial.print(location);
    Serial.print(" temp:");
    Serial.print(temperature);
    Serial.print(" icon:");
    Serial.print(icon);
    Serial.print(" humidity:");
    Serial.print(humidity);
    Serial.print(" pressure:");
    Serial.print(pressure);
    Serial.print(" uv:");
    Serial.print(uv);
    Serial.print(" wind:");
    Serial.println(wind);
#endif
  }

  void handleAa55NotificationPacket(const uint8_t* data, size_t len) {
    if (len < 7) {
      return;
    }

    uint8_t appId = data[4];
    uint8_t titleLen = data[5];
    size_t index = 6;
    if (index + titleLen >= len) {
      return;
    }

    String title;
    title.reserve(titleLen);
    for (uint8_t i = 0; i < titleLen && index < len; i++, index++) {
      title += (char)data[index];
    }

    if (index >= len) {
      return;
    }

    uint8_t messageLen = data[index++];
    if (index + messageLen > len) {
      return;
    }

    String message;
    message.reserve(messageLen);
    for (uint8_t i = 0; i < messageLen && index < len; i++, index++) {
      message += (char)data[index];
    }

#if ENABLE_SERIAL_DEBUG
    Serial.print("AA55 notification => app:");
    Serial.print(appId);
    Serial.print(" title:");
    Serial.print(title);
    Serial.print(" msg:");
    Serial.println(message);
#endif
  }

  void handleAa55BatteryPacket(const uint8_t* data, size_t len) {
    if (len < 6) {
      return;
    }

    uint8_t level = data[4];
    bool charging = data[5] != 0;

#if ENABLE_SERIAL_DEBUG
    Serial.print("AA55 battery => level:");
    Serial.print(level);
    Serial.print(" charging:");
    Serial.println(charging ? "yes" : "no");
#endif
  }

  static String decodeChronosTime(const uint8_t* data, size_t len) {
    // Chronos binary packets observed so far:
    // - type 0x93: ... [7][8]=year, [9]=month, [10]=day, trailing bytes carry time
    // - type 0x74: weather/status packet; some app versions also encode HH/MM digits
    // Keep display in HH:MM to match Chronos 24-hour behavior reliably.

    if (data == nullptr || len < 6) {
      return "";
    }

    auto format24 = [](uint8_t hour, uint8_t minute) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02u:%02u", hour, minute);
      return String(buf);
    };

    auto bcdToDec = [](uint8_t b) -> int {
      uint8_t hi = (b >> 4) & 0x0F;
      uint8_t lo = b & 0x0F;
      if (hi > 9 || lo > 9) {
        return -1;
      }
      return int(hi) * 10 + int(lo);
    };

    uint8_t packetType = data[4];

#if ENABLE_SERIAL_DEBUG
    Serial.print("Chronos packet (");
    Serial.print(len);
    Serial.print(" bytes): ");
    for (size_t i = 0; i < len && i < 20; i++) {
      if (data[i] < 0x10) Serial.print("0");
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
#endif

    // Packet 0x93 => explicit date-time packet.
    if (packetType == 0x93 && len > 12) {
      auto decodeField = [&](uint8_t raw, uint8_t limit) -> int {
        if (raw <= limit) {
          return (int)raw;
        }
        int bcd = bcdToDec(raw);
        if (bcd >= 0 && bcd <= limit) {
          return bcd;
        }
        return -1;
      };

      int hour = decodeField(data[11], 23);
      int minute = decodeField(data[12], 59);

      if (hour >= 0 && minute >= 0) {
        String timeStr = format24((uint8_t)hour, (uint8_t)minute);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos time [type 0x93 h=data[11], m=data[12]] => ");
        Serial.println(timeStr);
#endif
        return timeStr;
      }

      int swappedHour = decodeField(data[12], 23);
      int swappedMinute = decodeField(data[11], 59);
      if (swappedHour >= 0 && swappedMinute >= 0) {
        String timeStr = format24((uint8_t)swappedHour, (uint8_t)swappedMinute);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos time [type 0x93 fallback swapped] => ");
        Serial.println(timeStr);
#endif
        return timeStr;
      }

#if ENABLE_SERIAL_DEBUG
      Serial.println("Chronos time [type 0x93] => no valid candidate");
#endif
    }

    // For Chronos AB 00 packets, do not fall back to generic BCD scan.
    // It can decode random fields as fake times (e.g., 01:05 / 00:05).
    return "";
  }

  static String decodeChronosDate(const uint8_t* data, size_t len) {
    // Chronos packet type 0x93 carries explicit Y/M/D fields.
    String dateStr = "";

    if (data == nullptr || len < 6) {
      return dateStr;
    }

    uint8_t packetType = data[4];

    // Type 0x93: [7][8]=year (big endian), [9]=month, [10]=day
    if (packetType == 0x93 && len > 10) {
      int year = (int(data[7]) << 8) | int(data[8]);
      int month = data[9];
      int day = data[10];

      if (year >= 2000 && year <= 2099 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d", month, day, year);
        dateStr = buf;
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos date [type 0x93 Y/M/D] => ");
        Serial.println(dateStr);
#endif
        return dateStr;
      }
    }

        // For Chronos packets, only type 0x93 is treated as authoritative date payload.
    return dateStr;
  }

  static String decodeChronosWeather(const uint8_t* data, size_t len) {
    // Weather can come from multiple positions depending on Chronos app version/format
    // Try to extract weather condition text or code
    // Positions 12+ typically contain weather data (text or codes)
    
    String weatherStr = "";
    
    if (data == nullptr || len < 6) {
      return weatherStr;
    }

    uint8_t packetType = data[4];

    // Type 0x93 is date-time; ignore weather decode there.
    if (packetType == 0x93) {
      return "";
    }

    // Type 0x72 appears to carry a compact weather/status payload in some
    // Chronos app versions. Byte 5 often behaves like a weather code with
    // flag bits in the upper half, so mask them off before mapping.
    if (packetType == 0x72 && len > 5) {
      uint8_t weatherCode = data[5] & 0x7F;
      switch (weatherCode) {
        case 0: weatherStr = "Sunny"; break;
        case 1: weatherStr = "Cloudy"; break;
        case 2: weatherStr = "Rainy"; break;
        case 3: weatherStr = "Windy"; break;
        case 4: weatherStr = "Snow"; break;
        case 5: weatherStr = "Fog"; break;
        case 6: weatherStr = "Thunder"; break;
        case 7: weatherStr = "Clear"; break;
        case 8: weatherStr = "Overcast"; break;
        default:
          if (weatherCode < 20) {
            weatherStr = "Weather condition ";
            weatherStr += String(weatherCode);
          }
      }

      if (weatherStr.length() > 0) {
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos weather [type 0x72 compact] => ");
        Serial.println(weatherStr);
#endif
        return weatherStr;
      }
    }

    // Strategy 1: Extract ASCII weather text from positions 12+ onwards
    if (len > 12) {
      String weatherText = "";
      // Scan for printable characters that form weather description
      for (size_t i = 12; i < len && i < 24 && weatherText.length() < 20; i++) {
        uint8_t b = data[i];
        // Include printable ASCII
        if (b >= 32 && b <= 126) {
          weatherText += (char)b;
        } else if ((b == 0 || b == 0xFF) && weatherText.length() > 0) {
          // End marker found and we have text
          break;
        }
      }
      
      if (weatherText.length() > 0) {
        weatherText.trim();
        // Clean up any trailing spaces or special chars
        while (weatherText.length() > 0 && !isAlphaNumeric(weatherText.charAt(weatherText.length() - 1))) {
          weatherText.remove(weatherText.length() - 1);
        }
        
        if (weatherText.length() > 2) {
          weatherStr = weatherText;
#if ENABLE_SERIAL_DEBUG
          Serial.print("Chronos weather [pos 12+ ASCII] => ");
          Serial.println(weatherStr);
#endif
          return weatherStr;
        }
      }
    }
    
    // Strategy 2: Check position 12 for weather code (0=sunny, 1=cloudy, etc.)
    if (packetType == 0x74 && len > 12) {
      uint8_t weatherCode = data[12];
      // Map common weather codes
      switch (weatherCode) {
        case 0: weatherStr = "Sunny"; break;
        case 1: weatherStr = "Cloudy"; break;
        case 2: weatherStr = "Rainy"; break;
        case 3: weatherStr = "Windy"; break;
        case 4: weatherStr = "Snow"; break;
        case 5: weatherStr = "Fog"; break;
        case 6: weatherStr = "Thunder"; break;
        case 7: weatherStr = "Clear"; break;
        case 8: weatherStr = "Overcast"; break;
        default:
          // Check if code is in reasonable range
          if (weatherCode < 20) {
            weatherStr = "Weather condition ";
            weatherStr += String(weatherCode);
          }
      }
      
      if (weatherStr.length() > 0) {
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos weather [pos 12 code] => ");
        Serial.println(weatherStr);
#endif
        return weatherStr;
      }
    }

    return weatherStr;
  }

  static String decodeChronosTemperature(const uint8_t* data, size_t len) {
    // Temperature typically follows weather or is at a fixed position
    // Chronos app can send Celsius or Fahrenheit based on settings
    // Try multiple positions: [13], [14], or as two-byte signed value
    
    if (data == nullptr || len < 6) {
      return "";
    }

    uint8_t packetType = data[4];

    // Type 0x93/0x74 packets in Chronos commonly contain date/time/weather, not temperature.
    // Ignore these to avoid false values like 0°C from minute/flag bytes.
    if (packetType == 0x93 || packetType == 0x74) {
      return "";
    }

    // Type 0x72 compact packets often encode temperature directly in byte 6.
    // Example logs show values like 0x1B, 0x1C, 0x1D, 0x20 which map cleanly to
    // 27°C, 28°C, 29°C, 32°C.
    if (packetType == 0x72 && len > 6) {
      uint8_t tempByte = data[6];
      if (tempByte <= 60) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u°C", tempByte);
        String result = String(buf);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos temp [type 0x72 compact] => ");
        Serial.println(result);
#endif
        return result;
      }
    }

    // Very short packets are unlikely to carry temperature payload safely.
    if (len <= 14) {
      return "";
    }

    // Strategy 1: Check position 13 as direct temperature value
    if (len > 13) {
      int8_t temp = (int8_t)data[13];
      if (temp >= -50 && temp <= 60) {  // Valid range for Celsius
        char buf[16];
        // Display with °C by default (Chronos typically uses metric)
        snprintf(buf, sizeof(buf), "%d°C", temp);
        String result = String(buf);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos temp [pos 13 Celsius] => ");
        Serial.println(result);
#endif
        return result;
      }
    }

    // Strategy 2: Check position 14 as direct temperature value
    if (len > 14) {
      int8_t temp = (int8_t)data[14];
      if (temp >= -50 && temp <= 60) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d°C", temp);
        String result = String(buf);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos temp [pos 14 Celsius] => ");
        Serial.println(result);
#endif
        return result;
      }
    }

    // Strategy 3: Temperature as unsigned byte (0-100 range)
    if (len > 13) {
      uint8_t tempByte = data[13];
      if (tempByte <= 100) {  // Treat as Celsius 0-100
        char buf[16];
        snprintf(buf, sizeof(buf), "%u°C", tempByte);
        String result = String(buf);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos temp [pos 13 unsigned] => ");
        Serial.println(result);
#endif
        return result;
      }
    }

    // Strategy 4: Could be Fahrenheit (32-120 range for reasonable temps)
    if (len > 13) {
      int8_t tempF = (int8_t)data[13];
      if (tempF >= 32 && tempF <= 120) {
        // Could be Fahrenheit — convert to Celsius for display
        int tempC = (tempF - 32) * 5 / 9;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d°C", tempC);
        String result = String(buf);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos temp [pos 13 Fahrenheit->Celsius] => ");
        Serial.println(result);
#endif
        return result;
      }
    }

    return "";
  }

  static String guessTimeFromText(const String& source) {
    for (int i = 0; i < (int)source.length(); i++) {
      if (source[i] == ':') {
        int start = i - 2;
        int end = i + 3;
        if (start >= 0 && end <= (int)source.length()) {
          String candidate = source.substring(start, end);
          candidate.trim();
          return candidate;
        }
      }
    }
    return "";
  }

  static String guessDateFromText(const String& source) {
    String lowered = source;
    lowered.toLowerCase();

    const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
    for (int i = 0; i < 12; i++) {
      int idx = lowered.indexOf(months[i]);
      if (idx >= 0) {
        int start = max(0, idx - 4);
        int end = min((int)source.length(), idx + 10);
        String candidate = source.substring(start, end);
        candidate.trim();
        return candidate;
      }
    }

    int slash = source.indexOf('/');
    if (slash > 0) {
      int start = max(0, slash - 2);
      int end = min((int)source.length(), slash + 6);
      String candidate = source.substring(start, end);
      candidate.trim();
      return candidate;
    }
    return "";
  }

  static String guessWeatherFromText(const String& source) {
    String lowered = source;
    lowered.toLowerCase();
    const char* words[] = {"sun", "cloud", "rain", "storm", "clear", "mist", "fog", "haze", "snow", "wind", "humid"};
    for (int i = 0; i < 11; i++) {
      int idx = lowered.indexOf(words[i]);
      if (idx >= 0) {
        int start = max(0, idx - 6);
        int end = min((int)source.length(), idx + 14);
        String candidate = source.substring(start, end);
        candidate.trim();
        return candidate;
      }
    }
    return "";
  }

  void applyTextCommand(String command) {
    String rawCommand = command;
    String normalized = normalizeCommand(command);

    String timeValue = pickFirstNonEmpty(
      extractJsonValue(rawCommand, "time"),
      extractJsonValue(rawCommand, "clock"),
      extractKeyValue(rawCommand, "time"),
      extractKeyValue(rawCommand, "clock")
    );

    String dateValue = pickFirstNonEmpty(
      extractJsonValue(rawCommand, "date"),
      extractJsonValue(rawCommand, "day"),
      extractKeyValue(rawCommand, "date"),
      extractKeyValue(rawCommand, "day")
    );

    String weatherValue = pickFirstNonEmpty(
      extractJsonValue(rawCommand, "weather"),
      extractJsonValue(rawCommand, "condition"),
      extractKeyValue(rawCommand, "weather"),
      extractKeyValue(rawCommand, "condition")
    );

    if (timeValue.length() == 0) {
      timeValue = guessTimeFromText(rawCommand);
    }
    if (dateValue.length() == 0) {
      dateValue = guessDateFromText(rawCommand);
    }
    if (weatherValue.length() == 0) {
      weatherValue = guessWeatherFromText(rawCommand);
    }

    if (timeValue.length() > 0 || dateValue.length() > 0 || weatherValue.length() > 0) {
      stateMachine->setChronosInfo(timeValue, dateValue, weatherValue);
#if ENABLE_SERIAL_DEBUG
      Serial.print("Chronos data parsed => time:");
      Serial.print(timeValue);
      Serial.print(" date:");
      Serial.print(dateValue);
      Serial.print(" weather:");
      Serial.println(weatherValue);
#endif
    }

    if (normalized.indexOf("mode\":\"info") >= 0 || normalized.indexOf("show_info") >= 0 || normalized.indexOf("info_mode") >= 0) {
      stateMachine->setBluetoothConnected(true);
      stateMachine->setInfoMode(true);
      return;
    }

    if (normalized.indexOf("mode\":\"animation") >= 0 || normalized.indexOf("animations") >= 0) {
      stateMachine->setBluetoothConnected(true);
      stateMachine->setInfoMode(false);
      return;
    }

    if (normalized.indexOf("sleep") >= 0) {
      stateMachine->sleepNow();
      return;
    }

    if (normalized.indexOf("wake") >= 0) {
      stateMachine->wakeUp();
      return;
    }

    if (normalized.indexOf("angry") >= 0 || normalized.indexOf("confused") >= 0) {
      stateMachine->triggerEmotion(CONFUSED);
      return;
    }

    if (normalized.indexOf("love") >= 0 || normalized.indexOf("happy") >= 0) {
      stateMachine->triggerEmotion(HAPPY);
      return;
    }

    if (normalized.indexOf("game") >= 0) {
      stateMachine->triggerEmotion(CONFUSED);
      return;
    }

    if (normalized.indexOf("surprised") >= 0) {
      stateMachine->triggerEmotion(SURPRISED);
      return;
    }

    if (normalized.indexOf("blink") >= 0) {
      stateMachine->triggerEmotion(BLINK);
      return;
    }
  }

  void applyBinaryCommand(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
      return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.print("BLE RX HEX: ");
    Serial.println(bytesToHex(data, len));
#endif

    if (isAa55Packet(data, len)) {
#if ENABLE_SERIAL_DEBUG
      Serial.println("-> Detected AA55 Chronos packet");
      if (len > 3) {
        Serial.print("-> AA55 command: 0x");
        Serial.println(data[2], HEX);
      }
#endif
      uint8_t command = data[2];
      switch (command) {
        case 0x01:
          handleAa55TimePacket(data, len);
          return;
        case 0x02:
          handleAa55WeatherPacket(data, len);
          return;
        case 0x03:
          handleAa55NotificationPacket(data, len);
          return;
        case 0x04:
#if ENABLE_SERIAL_DEBUG
          Serial.println("AA55 navigation packet received");
#endif
          return;
        case 0x05:
          handleAa55BatteryPacket(data, len);
          return;
        default:
#if ENABLE_SERIAL_DEBUG
          Serial.println("AA55 packet => unhandled command");
#endif
          return;
      }
    }

    bool isChronosPacket = (len >= 4 && data[0] == 0xAB && data[1] == 0x00);

    // Check if payload header matches Chronos protocol (starts with AB 00)
    if (isChronosPacket) {
#if ENABLE_SERIAL_DEBUG
      Serial.println("-> Detected Chronos protocol packet");
      if (len > 4) {
        Serial.print("-> Chronos packet type: 0x");
        Serial.println(data[4], HEX);
      }
#endif
      
      String time = decodeChronosTime(data, len);
      String date = decodeChronosDate(data, len);
      String weather = decodeChronosWeather(data, len);
      String temperature = decodeChronosTemperature(data, len);
      
      if (time.length() > 0 || date.length() > 0 || weather.length() > 0 || temperature.length() > 0) {
        String location = "";
        if (data[4] == 0x72 || data[4] == 0x74) {
          location = "Bhilai";
        }

        stateMachine->setChronosInfo(time, date, weather, temperature, location);
#if ENABLE_SERIAL_DEBUG
        Serial.print("Chronos parsed => time:");
        Serial.print(time);
        Serial.print(" date:");
        Serial.print(date);
        Serial.print(" weather:");
        Serial.print(weather);
        Serial.print(" temp:");
        Serial.println(temperature);
#endif
        return;
      }

#if ENABLE_SERIAL_DEBUG
      Serial.println("Chronos parsed => no recognized fields in this packet");
#endif
      // IMPORTANT: do not run generic ASCII/BCD parsers for Chronos packets.
      // They can misread random bytes as fake times/weather.
      return;
    }

    // Try ASCII-like extraction first (some apps wrap readable payloads in binary frames).
    String looseText = bytesToAsciiLoose(data, len);
    looseText.trim();
    if (looseText.length() > 0) {
      applyTextCommand(looseText);
    }

    // Try direct text parse when payload is mostly printable.
    if (isMostlyPrintable(data, len)) {
      String plain;
      plain.reserve((int)len);
      for (size_t i = 0; i < len; i++) {
        plain += (char)data[i];
      }
      applyTextCommand(plain);
    }

    // Try BCD HH:MM extraction common in wearable protocols.
    String bcdTime;
    if (decodeBcdTime(data, len, bcdTime)) {
      stateMachine->setChronosInfo(bcdTime, "", "");
#if ENABLE_SERIAL_DEBUG
      Serial.print("Chronos BCD time => ");
      Serial.println(bcdTime);
#endif
    }
  }

public:
  BleControl() = default;

  void begin(StateMachine* machine) {
    stateMachine = machine;

    BLEDevice::init(BLE_DEVICE_NAME);
    server = BLEDevice::createServer();
    server->setCallbacks(new BleServerCallbacks(this));
    service = server->createService(SERVICE_UUID);

    statusCharacteristic = service->createCharacteristic(
      STATUS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    statusCharacteristic->addDescriptor(new BLE2902());
    statusCharacteristic->setCallbacks(new BleCommandCallbacks(this));

    commandCharacteristic = service->createCharacteristic(
      COMMAND_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    commandCharacteristic->setCallbacks(new BleCommandCallbacks(this));

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    started = true;
    connected = false;
    lastStatus = "";
    lastStatusNotifyAt = 0;

    if (stateMachine != nullptr) {
      stateMachine->setBluetoothConnected(false);
    }

    Serial.print("BLE ready: ");
    Serial.println(BLE_DEVICE_NAME);
  }

  void update() {
    if (!started || stateMachine == nullptr || statusCharacteristic == nullptr) {
      return;
    }

    unsigned long now = millis();
    String status = stateMachine->statusJson();
    if (status != lastStatus || (now - lastStatusNotifyAt) >= STATUS_NOTIFY_INTERVAL_MS) {
      lastStatus = status;
      statusCharacteristic->setValue((uint8_t*)status.c_str(), status.length());
      statusCharacteristic->notify();
      lastStatusNotifyAt = now;
    }
  }

  void handleCommand(String command) {
    if (stateMachine == nullptr) {
      return;
    }
#if ENABLE_SERIAL_DEBUG
    Serial.print("BLE RX: ");
    Serial.println(command);
#endif
    applyTextCommand(command);
  }

  void handleCommandBytes(const uint8_t* data, size_t len) {
    if (stateMachine == nullptr) {
      return;
    }
    applyBinaryCommand(data, len);
  }

  bool isStarted() const {
    return started;
  }

  bool isConnected() const {
    return connected;
  }

  void setConnected(bool value) {
    connected = value;
    if (stateMachine != nullptr) {
      stateMachine->setBluetoothConnected(value);
    }
  }

  friend class BleCommandCallbacks;
  friend class BleServerCallbacks;
};

const char* BleControl::SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BleControl::STATUS_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BleControl::COMMAND_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

inline void BleCommandCallbacks::onWrite(BLECharacteristic* characteristic) {
  if (owner == nullptr) {
    return;
  }

  uint8_t* raw = characteristic->getData();
  size_t len = characteristic->getLength();
  if (raw != nullptr && len > 0) {
    owner->handleCommandBytes(raw, len);
    return;
  }

  String value = characteristic->getValue();
  if (value.length() > 0) {
    owner->handleCommand(value);
  }
}

inline void BleServerCallbacks::onConnect(BLEServer* server) {
  (void)server;
  if (owner != nullptr) {
    owner->setConnected(true);
#if ENABLE_SERIAL_DEBUG
    Serial.println("BLE connected");
#endif
  }
}

inline void BleServerCallbacks::onDisconnect(BLEServer* server) {
  (void)server;
  if (owner != nullptr) {
    owner->setConnected(false);
#if ENABLE_SERIAL_DEBUG
    Serial.println("BLE disconnected");
#endif
  }
  BLEDevice::startAdvertising();
}

#endif

#endif // BLE_CONTROL_H