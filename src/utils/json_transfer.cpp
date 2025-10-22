#include "../include/utils/json_transfer.h"

/*
    Transfer data to json format
*/

namespace JsonTransferData {
    String buildConnectPayload(const char* deviceId, const char* status) {
        StaticJsonDocument<1024> doc;
        doc["deviceId"] = deviceId;
        doc["status"] = status; 
        doc["timestamps"] = millis();

        String res;
        serializeJson(doc, res);
        return res;
    }

    String buildResponsePayload(const char* deviceId, const char* userId, const char* message, const char* status) {
        StaticJsonDocument<1024> doc;
        doc["deviceId"] = deviceId;
        doc["userId"] = userId;
        doc["message"] = message;
        doc["status"] = status;
        doc["timestamps"] = millis();

        String res;
        serializeJson(doc, res);
        return res;
    }

    String buildRequestPayload();

}