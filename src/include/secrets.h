#pragma

// Wifi connection
#define WIFI_SSID "Bamboo302"
#define WIFI_PASSWORD "Bkapro95@"

// MQTT connection
#define MQTT_BROKER_URL "2f768b73d7604f2d9497f4ed5c04f376.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USERNAME "Hung091103"
#define MQTT_PASSWORD "Hung091103"
#define MQTT_CLIENT_ID "ESP32ClientID"

// OTA Configuration
#define OTA_SERVER_URL "http://192.168.1.100:8000"
#define FIRMWARE_VERSION "1.0.0"
#define OTA_CHECK_INTERVAL 3600000  // 1 hour in milliseconds

// DEVICE CERT
#define LED_PIN 2

// L298N Motor pins
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENA 25
#define ENB 33

#define SERVICE_UUID        "a6decdd8-f8af-49cc-91f9-5a0b43283a6e"
#define CHARACTERISTIC_UUID "d6e61f51-671e-41e1-b865-11c553ef86e2"
