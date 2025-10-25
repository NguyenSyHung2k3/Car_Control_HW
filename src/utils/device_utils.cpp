#include "../include/utils/device_utils.h"
#include <WiFi.h>

namespace DeviceUtils {
    String getMacAddress() {
        WiFi.mode(WIFI_STA);
        return WiFi.macAddress();
    }
}