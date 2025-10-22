#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>

namespace JsonTransferData {
    String buildConnectPayload(const char* deviceId, const char* status);
    String buildResponsePayload(const char* deviceId, const char* userId, const char* message, const char* status);
    String buildRequestPayload();
}