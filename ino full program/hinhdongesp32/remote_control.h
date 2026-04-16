#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
using MochiWebServer = WebServer;
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using MochiWebServer = ESP8266WebServer;
#endif
#include "config.h"
#include "state_machine.h"

class RemoteControl {
private:
  MochiWebServer server;
  StateMachine* stateMachine = nullptr;
  bool started = false;

  String htmlPage() const {
    String page;
    page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<style>body{font-family:sans-serif;background:#111;color:#fff;padding:18px}button{width:100%;padding:16px;margin:8px 0;border:0;border-radius:14px;font-size:18px} .g{background:#2ecc71} .y{background:#f1c40f} .r{background:#e74c3c} .b{background:#3498db}</style></head><body>");
    page += F("<h2>Dasai Mochi</h2><p>Phone-friendly control panel</p>");
    page += F("<button class='g' onclick=\"fetch('/api/emotion?name=happy')\">Happy</button>");
    page += F("<button class='b' onclick=\"fetch('/api/emotion?name=surprised')\">Surprised</button>");
    page += F("<button class='y' onclick=\"fetch('/api/emotion?name=sleepy')\">Sleepy</button>");
    page += F("<button class='r' onclick=\"fetch('/api/state?mode=sleep')\">Sleep</button>");
    page += F("<button class='g' onclick=\"fetch('/api/state?mode=idle')\">Idle</button>");
    page += F("<button class='b' onclick=\"fetch('/api/state?mode=wake')\">Wake</button>");
    page += F("<button class='y' onclick=\"fetch('/api/emotion?name=confused')\">Confused</button>");
    page += F("<button class='g' onclick=\"fetch('/api/emotion?name=blink')\">Blink</button>");
    page += F("<p><a href='/api/status'>Status JSON</a></p>");
    page += F("</body></html>");
    return page;
  }

  void handleRoot() {
    server.send(200, "text/html", htmlPage());
  }

  void handleStatus() {
    server.send(200, "application/json", stateMachine->statusJson());
  }

  void handleEmotion() {
    String emotionName = server.arg("name");
    stateMachine->setEmotionByName(emotionName);
    server.send(200, "application/json", stateMachine->statusJson());
  }

  void handleState() {
    String mode = server.arg("mode");
    mode.toLowerCase();

    if (mode == "sleep") {
      stateMachine->sleepNow();
    } else if (mode == "wake") {
      stateMachine->wakeUp();
    } else {
      stateMachine->returnToIdle();
    }

    server.send(200, "application/json", stateMachine->statusJson());
  }

  void handleBrightness() {
    if (server.hasArg("value")) {
      int brightness = constrain(server.arg("value").toInt(), 0, 255);
      stateMachine->setBrightness(brightness);
    }
    server.send(200, "application/json", stateMachine->statusJson());
  }

#if ENABLE_CHRONOS_COMPAT
  void handleChronosStatus() {
    String json = "{";
    json += "\"device_id\":\"" + String(CHRONOS_DEVICE_ID) + "\",";
    json += "\"name\":\"Dasai Mochi\",";
    json += "\"type\":\"animation_character\",";
    json += "\"version\":\"1.0.0\",";
    json += "\"state\":\"" + String(stateMachine->getStateName()) + "\",";
    json += "\"emotion\":\"" + String(stateMachine->getEmotionName()) + "\",";
    json += "\"brightness\":255,";
    json += "\"sound_reactive\":true,";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"features\":[\"animations\",\"emotions\",\"sound_reactive\",\"touch_sensors\",\"button_control\"]";
    json += "}";
    server.send(200, "application/json", json);
  }

  void handleChronosDevice() {
    String json = "{";
    json += "\"id\":\"" + String(CHRONOS_DEVICE_ID) + "\",";
    json += "\"name\":\"Dasai Mochi\",";
    json += "\"type\":\"animation_character\",";
    json += "\"manufacturer\":\"Dasai_Corp\",";
    json += "\"model\":\"Mochi_ESP32_v1\",";
    json += "\"firmware\":\"1.0.0\",";
    json += "\"hardware\":\"ESP32_DevKit\",";
    json += "\"display\":\"SSD1306_128x64\",";
    json += "\"microphone\":\"INMP441\",";
    json += "\"connected\":true,";
    json += "\"network\":{\"ssid\":\"" + String(WIFI_AP_SSID) + "\",\"ip\":\"192.168.4.1\"},";
    json += "\"capabilities\":{";
    json += "\"emotions\":[\"idle\",\"happy\",\"sleepy\",\"surprised\",\"confused\",\"blink\"],";
    json += "\"states\":[\"idle\",\"interact\",\"sleep\",\"wake\",\"emotion\"],";
    json += "\"features\":[\"animation\",\"sound_reactive\",\"touch_input\",\"button_control\",\"brightness_control\"],";
    json += "\"sensors\":[\"microphone\",\"touch_left\",\"touch_right\"]";
    json += "}";
    json += "}";
    server.send(200, "application/json", json);
  }

  void handleChronosCommand() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"no body\"}");
      return;
    }

    String body = server.arg("plain");
    String action = "";
    String value = "";
    
    if (body.indexOf("\"action\"") >= 0) {
      int start = body.indexOf("\"action\":\"") + 10;
      int end = body.indexOf("\"", start);
      action = body.substring(start, end);
    }
    
    if (body.indexOf("\"value\"") >= 0) {
      int start = body.indexOf("\"value\":\"") + 9;
      int end = body.indexOf("\"", start);
      value = body.substring(start, end);
    }

    bool success = true;

    if (action == "emotion") {
      if (value == "happy" || value == "love") {
        stateMachine->triggerEmotion(HAPPY);
      } else if (value == "angry" || value == "confused") {
        stateMachine->triggerEmotion(CONFUSED);
      } else if (value == "sleepy" || value == "sleep") {
        stateMachine->sleepNow();
      } else if (value == "surprised" || value == "shock") {
        stateMachine->triggerEmotion(SURPRISED);
      } else if (value == "blink") {
        stateMachine->triggerEmotion(BLINK);
      } else {
        success = false;
      }
    } else if (action == "state") {
      if (value == "sleep") {
        stateMachine->sleepNow();
      } else if (value == "wake") {
        stateMachine->wakeUp();
      } else if (value == "idle") {
        stateMachine->returnToIdle();
      } else {
        success = false;
      }
    } else if (action == "brightness") {
      int brightness = value.toInt();
      stateMachine->setBrightness(brightness);
    } else {
      success = false;
    }

    String response = "{\"status\":\"" + String(success ? "ok" : "error") + "\",";
    response += "\"action\":\"" + action + "\",";
    response += "\"value\":\"" + value + "\",";
    response += "\"device_state\":\"" + String(stateMachine->getStateName()) + "\",";
    response += "\"emotion\":\"" + String(stateMachine->getEmotionName()) + "\"}";
    
    server.send(success ? 200 : 400, "application/json", response);
  }

  void handleChronosHeartbeat() {
    String json = "{";
    json += "\"device_id\":\"" + String(CHRONOS_DEVICE_ID) + "\",";
    json += "\"status\":\"online\",";
    json += "\"timestamp\":" + String(millis() / 1000) + ",";
    json += "\"state\":\"" + String(stateMachine->getStateName()) + "\"";
    json += "}";
    server.send(200, "application/json", json);
  }
#endif

public:
  RemoteControl() : server(WIFI_HTTP_PORT) {}

  bool begin(StateMachine* machine) {
    stateMachine = machine;

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  #if defined(ARDUINO_ARCH_ESP8266)
    WiFi.hostname(WIFI_HOSTNAME);
  #endif

    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/emotion", HTTP_GET, [this]() { handleEmotion(); });
    server.on("/api/state", HTTP_GET, [this]() { handleState(); });
    server.on("/api/brightness", HTTP_GET, [this]() { handleBrightness(); });
    
#if ENABLE_CHRONOS_COMPAT
    server.on("/chronos/status", HTTP_GET, [this]() { handleChronosStatus(); });
    server.on("/chronos/device", HTTP_GET, [this]() { handleChronosDevice(); });
    server.on("/chronos/command", HTTP_POST, [this]() { handleChronosCommand(); });
    server.on("/chronos/heartbeat", HTTP_GET, [this]() { handleChronosHeartbeat(); });
#endif

    server.begin();
    started = true;

    Serial.print("WiFi AP ready: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    return true;
  }

  void update() {
    if (started) {
      server.handleClient();
    }
  }

  bool isStarted() const {
    return started;
  }
};

#endif // REMOTE_CONTROL_H
