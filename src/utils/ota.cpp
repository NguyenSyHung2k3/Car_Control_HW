#include "include/utils/ota.h"
#include "include/mqtt_module.h"
#include "include/utils/device_utils.h"
#include "include/secrets.h"

// Global instance for MQTT integration
OTAManager* g_otaManager = nullptr;

OTAManager::OTAManager(const String& serverUrl) 
    : serverUrl(serverUrl), isUpdating(false), lastCheckTime(0), checkInterval(OTA_CHECK_INTERVAL) {
    deviceId = DeviceUtils::getMacAddress();
}

void OTAManager::begin() {
    deviceId = DeviceUtils::getMacAddress();
    g_otaManager = this;
}

void OTAManager::loop() {
    if (isUpdating) {
        return; // Don't check for updates while updating
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastCheckTime >= checkInterval) {
        checkForUpdates();
        lastCheckTime = currentTime;
    }
}

void OTAManager::checkForUpdates() {
    if (isUpdating) {
        Serial.println("[OTA] Update already in progress");
        return;
    }
    
    Serial.println("[OTA] Checking for updates...");
    
    if (checkForUpdate()) {
        Serial.println("[OTA] Update available, downloading...");
    } else {
        Serial.println("[OTA] No updates available");
    }
}

bool OTAManager::checkForUpdate() {
    if (!WiFi.isConnected()) {
        Serial.println("[OTA] WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    String checkUrl = serverUrl + "/api/ota/check/" + deviceId;
    
    http.begin(checkUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Send current firmware version
    JsonDocument requestDoc;
    requestDoc["deviceId"] = deviceId;
    requestDoc["currentVersion"] = FIRMWARE_VERSION;
    
    String requestBody;
    serializeJson(requestDoc, requestBody);
    
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        http.end();
        
        JsonDocument responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response);
        
        if (error) {
            Serial.printf("[OTA] JSON parse error: %s\n", error.c_str());
            return false;
        }
        
        bool updateAvailable = responseDoc["updateAvailable"];
        if (updateAvailable) {
            String firmwareUrl = responseDoc["firmwareUrl"];
            String version = responseDoc["version"];
            
            Serial.printf("[OTA] Update available: %s\n", version.c_str());
            Serial.printf("[OTA] Firmware URL: %s\n", firmwareUrl.c_str());
            
            sendUpdateStatus("available", "Update available: " + version);
            
            // Auto-download and install
            return downloadAndInstallUpdate(firmwareUrl);
        } else {
            Serial.println("[OTA] Device is up to date");
            return false;
        }
    } else {
        Serial.printf("[OTA] HTTP error: %d\n", httpResponseCode);
        http.end();
        return false;
    }
}

bool OTAManager::downloadAndInstallUpdate(const String& firmwareUrl) {
    if (isUpdating) {
        Serial.println("[OTA] Update already in progress");
        return false;
    }
    
    if (!WiFi.isConnected()) {
        Serial.println("[OTA] WiFi not connected");
        return false;
    }
    
    isUpdating = true;
    sendUpdateStatus("downloading", "Downloading firmware...");
    
    HTTPClient http;
    http.begin(firmwareUrl);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode != 200) {
        Serial.printf("[OTA] Download failed, HTTP error: %d\n", httpResponseCode);
        http.end();
        isUpdating = false;
        sendUpdateStatus("failed", "Download failed");
        return false;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("[OTA] Invalid content length");
        http.end();
        isUpdating = false;
        sendUpdateStatus("failed", "Invalid firmware size");
        return false;
    }
    
    Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);
    
    // Check if we have enough space
    if (!Update.begin(contentLength)) {
        Serial.printf("[OTA] Not enough space for update. Error: %s\n", Update.errorString());
        http.end();
        isUpdating = false;
        sendUpdateStatus("failed", "Not enough space");
        return false;
    }
    
    sendUpdateStatus("installing", "Installing firmware...");
    
    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[128] = { 0 };
    
    while (http.connected() && (written < contentLength)) {
        size_t size = stream->available();
        if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            if (c > 0) {
                Update.write(buff, c);
                written += c;
                
                // Progress indicator
                if (written % 1024 == 0) {
                    Serial.printf("[OTA] Progress: %d/%d bytes (%.1f%%)\n", 
                                written, contentLength, (written * 100.0 / contentLength));
                }
            }
        }
        delay(1);
    }
    
    http.end();
    
    if (written == contentLength) {
        Serial.println("[OTA] Firmware download completed");
        
        if (Update.end()) {
            Serial.println("[OTA] Firmware update completed successfully");
            sendUpdateStatus("completed", "Update completed successfully");
            
            Serial.println("[OTA] Restarting device...");
            delay(1000);
            ESP.restart();
            return true;
        } else {
            Serial.printf("[OTA] Update failed: %s\n", Update.errorString());
            sendUpdateStatus("failed", "Installation failed");
            isUpdating = false;
            return false;
        }
    } else {
        Serial.printf("[OTA] Download incomplete: %d/%d bytes\n", written, contentLength);
        Update.abort();
        sendUpdateStatus("failed", "Download incomplete");
        isUpdating = false;
        return false;
    }
}

void OTAManager::forceUpdate(const String& firmwareUrl) {
    Serial.printf("[OTA] Force update requested: %s\n", firmwareUrl.c_str());
    downloadAndInstallUpdate(firmwareUrl);
}

bool OTAManager::isCurrentlyUpdating() const {
    return isUpdating;
}

void OTAManager::setCheckInterval(unsigned long interval) {
    checkInterval = interval;
    Serial.printf("[OTA] Check interval set to %lu ms\n", interval);
}

void OTAManager::sendUpdateStatus(const String& status, const String& message) {
    // Send status via MQTT if available
    if (g_isConnected && !g_userId.isEmpty()) {
        MqttClient* mqtt = MqttClient::getInstance();
        if (mqtt) {
            String deviceId = DeviceUtils::getMacAddress();
            String topic = String("iot/") + g_userId + "/" + deviceId + "/ota/status";
            
            JsonDocument statusDoc;
            statusDoc["status"] = status;
            statusDoc["message"] = message;
            statusDoc["timestamp"] = millis();
            
            String payload;
            serializeJson(statusDoc, payload);
            
            mqtt->publish(topic.c_str(), payload.c_str());
            Serial.printf("[OTA] Status sent: %s - %s\n", status.c_str(), message.c_str());
        }
    }
}

// Static callback for MQTT integration
void OTAManager::handleOTACommand(const String& command, const String& firmwareUrl) {
    if (!g_otaManager) {
        Serial.println("[OTA] OTA Manager not initialized");
        return;
    }
    
    if (command == "CHECK_UPDATE") {
        Serial.println("[OTA] Manual update check requested");
        g_otaManager->checkForUpdates();
    } else if (command == "FORCE_UPDATE" && firmwareUrl.length() > 0) {
        Serial.printf("[OTA] Force update requested: %s\n", firmwareUrl.c_str());
        g_otaManager->forceUpdate(firmwareUrl);
    } else {
        Serial.printf("[OTA] Unknown OTA command: %s\n", command.c_str());
    }
}
