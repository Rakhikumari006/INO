/*
 * CONFIG.H - Dasai Mochi personality and control settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// Hardware
#if defined(ARDUINO_ARCH_ESP32)
#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_PIN 26
#define TOUCH_LEFT_PIN 27
#define TOUCH_RIGHT_PIN 14
#define TOUCH_ACTIVE_HIGH 1
#define SENSOR_PIN 34
#else
#define SDA_PIN D2
#define SCL_PIN D1
#define BUTTON_PIN D3
#define TOUCH_LEFT_PIN D5
#define TOUCH_RIGHT_PIN D6
#define TOUCH_ACTIVE_HIGH 1
#define SENSOR_PIN A0
#endif
#define I2C_CLOCK 400000

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define DISPLAY_IS_SH1106 1
#define BRIGHTNESS_NORMAL 255
#define BRIGHTNESS_DIM 120
#define BRIGHTNESS_SLEEP 45
#define FRAME_DATA_IS_XBM 0

// State timing
#define SLEEP_TIMEOUT 300000
#define SENSOR_IDLE_SLEEP_TIMEOUT 300000
#define PRE_SLEEPY_TIMEOUT 240000
#define INTERACT_DURATION 4500
#define EMOTION_DURATION 7000
#define WAKE_DURATION 2800

// Input timing
#define DEBOUNCE_TIME 45
#define LONG_PRESS_TIME 900
#define DOUBLE_TAP_WINDOW 350

// Personality timing
#define IDLE_BASE_SPEED 72
#define HAPPY_SPEED 66
#define SLEEPY_SPEED 92
#define SURPRISED_SPEED 74
#define BLINK_SPEED 46
#define CONFUSED_SPEED 80

#define BLINK_MIN_INTERVAL 4000
#define BLINK_MAX_INTERVAL 9000
#define LOOK_MIN_INTERVAL 3000
#define LOOK_MAX_INTERVAL 7000
#define IDLE_PAUSE_MIN 120
#define IDLE_PAUSE_MAX 360

// Frame ranges
#define FRAME_IDLE_START 0
#define FRAME_IDLE_END 127
#define FRAME_HAPPY_START 128
#define FRAME_HAPPY_END 255
#define FRAME_SLEEPY_START 256
#define FRAME_SLEEPY_END 383
#define FRAME_SURPRISED_START 384
#define FRAME_SURPRISED_END 511
#define FRAME_BLINK_START 512
#define FRAME_BLINK_END 639
#define FRAME_CONFUSED_START 697
#define FRAME_CONFUSED_END 771

// Control / connectivity
#define ENABLE_SERIAL_DEBUG 1
#define ENABLE_WIFI_CONTROL 0
#define ENABLE_BLE_CONTROL 1
#define ENABLE_NTP_CLOCK 0
#define ENABLE_WEATHER_API 0
#define ENABLE_MOTION_SENSOR 0
#define ENABLE_SOUND_REACTIVE 0
#define ENABLE_CHRONOS_COMPAT 0

// Bluetooth / app control
#define BLE_DEVICE_NAME "DasaiMochi"

// OpenWeatherMap weather settings
// Replace this API key with your own if needed.
#define OPENWEATHER_API_KEY ""

// Choose one location mode:
// 1) City query: set WEATHER_QUERY and leave WEATHER_LATITUDE/WEATHER_LONGITUDE unused.
// 2) Coordinates: set WEATHER_LATITUDE and WEATHER_LONGITUDE to your exact location.
#define WEATHER_QUERY "Bhilai"
#define WEATHER_LATITUDE "kracken5G"
#define WEATHER_LONGITUDE "6260302033"
#define WEATHER_USE_COORDINATES 0

// Wi-Fi credentials used only for weather refresh
#define WEATHER_WIFI_SSID ""
#define WEATHER_WIFI_PASSWORD ""

// Weather refresh interval in milliseconds
#define WEATHER_UPDATE_INTERVAL 1800000UL

// Optional network credentials for station mode
// #define WIFI_STA_SSID "your-wifi"
// #define WIFI_STA_PASSWORD "your-password"

// Advanced sensor thresholds
#define MOTION_THRESHOLD 120
#define LIGHT_THRESHOLD 180
#if defined(ARDUINO_ARCH_ESP32)
#define AUDIO_THRESHOLD 450
#else
#define AUDIO_THRESHOLD 150
#endif

#endif // CONFIG_H
