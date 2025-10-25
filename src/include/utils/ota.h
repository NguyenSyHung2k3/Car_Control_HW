#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

class OTAManager {
private:
    String serverUrl;
    String deviceId;
    bool isUpdating;
    unsigned long lastCheckTime;
    unsigned long checkInterval;
    
    bool checkForUpdate();
    bool downloadAndInstallUpdate(const String& firmwareUrl);
    void sendUpdateStatus(const String& status, const String& message = "");
    
public:
    OTAManager(const String& serverUrl);
    
    void begin();
    void loop();
    void checkForUpdates();
    void forceUpdate(const String& firmwareUrl);
    bool isCurrentlyUpdating() const;
    void setCheckInterval(unsigned long interval);
    
    // Static callback for MQTT integration
    static void handleOTACommand(const String& command, const String& firmwareUrl = "");
};
