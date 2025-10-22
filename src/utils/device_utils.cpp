#include "../include/utils/device_utils.h"
#include <WiFi.h>

namespace DeviceUtils {
    String getMacAddress() {
        WiFi.mode(WIFI_STA);
        return WiFi.macAddress();
    }

    void handleLedOn() {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("[DEVICE] LED ON");
    }

    void handleLedOff() {
        digitalWrite(LED_PIN, LOW);
        Serial.println("[DEVICE] LED OFF");
    }
}