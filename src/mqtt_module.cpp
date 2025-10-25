#include "include/mqtt_module.h"
#include "include/utils/device_utils.h"
#include "include/utils/ota.h"

bool g_isConnected = false;
String g_userId = "";
MqttClient* MqttClient::instance = nullptr;

MqttClient::MqttClient(const char* broker, int port, const char* clientId)
    : broker(broker), port(port), clientId(clientId), mqttClient(espClient), connected(false), 
      isListeningMode(true), lastPublishTime(0), publishInterval(5000), 
      lastConnectionCheck(0), connectionCheckInterval(10000) {
    mqttClient.setServer(broker, port);

    espClient.setInsecure();

}

// Singleton getInstance
MqttClient* MqttClient::getInstance(const char* broker, int port, const char* clientId) {
    if (instance == nullptr) {
        instance = new MqttClient(broker, port, clientId);
    }
    return instance;
}

void MqttClient::connect() {
    while (!mqttClient.connected()){
        if (mqttClient.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD)) {
            connected = true;
        } else {
            delay(2000);
        }
    }
}

void MqttClient::loop() {
    mqttClient.loop();
    
    // Check connection status periodically
    checkConnectionStatus();
    
    // If in listening mode, continuously publish and subscribe
    if (isListeningMode && !g_isConnected) {
        unsigned long currentTime = millis();
        
        // Publish device status periodically
        if (currentTime - lastPublishTime >= publishInterval) {
            connectionPublish();
            lastPublishTime = currentTime;
        }
        
        // Ensure we're subscribed to connection topic
        connectionSubscribe();
    }
}

void MqttClient::publish(const char* topic, const char* payload){
    if(mqttClient.connected()){
        mqttClient.publish(topic, payload);
    }
}

void MqttClient::subscribe(const char* topic) {
    if(mqttClient.connected()){
        mqttClient.subscribe(topic);
    }
}

void MqttClient::connectionPublish() {
    if(!g_isConnected && isListeningMode) {
        String deviceId = DeviceUtils::getMacAddress();
        String topic = String("iot/") + deviceId + "/status";
        
        Serial.printf("[MQTT] Published to topic: %s\n", topic.c_str());

        // Create JSON payload manually
        JsonDocument doc;
        doc["deviceId"] = deviceId;
        doc["status"] = "Connected";
        
        String payload;
        serializeJson(doc, payload);

        publish(topic.c_str(), payload.c_str());

    }
}

void MqttClient::connectionSubscribe() {
    if(!g_isConnected && isListeningMode) {
        String deviceId = DeviceUtils::getMacAddress();
        String topic = String("iot/") + deviceId + "/connected";

        mqttClient.subscribe(topic.c_str());
        Serial.printf("[MQTT] Subscribed to topic: %s\n", topic.c_str());
        
        if(g_userId == ""){
            mqttClient.setCallback([this, deviceId](char* topic, byte* payload, unsigned int length) {
                String message;
                for (unsigned int i = 0; i < length; i++) {
                    message += (char)payload[i];
                }

                Serial.printf("[MQTT] Received on %s: %s\n", topic, message.c_str());

                // Parse JSON payload
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);
                if (error) {
                    Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
                    return;
                }

                const char* userId = doc["userId"];
                const char* msg = doc["msg"];

                if (userId && msg && String(msg) == "Connected") {
                    g_userId = String(userId);
                    g_isConnected = true;

                    Serial.printf("[DEVICE] Connected confirmed by server. userId=%s\n", userId);

                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/status";
                    JsonDocument resp;
                    resp["status"] = "OK";

                    String payloadStr;
                    serializeJson(resp, payloadStr);

                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                    
                    // Stop continuous listening to save power
                    stopContinuousListening();
                }
            });
        } else {
            String deviceId = DeviceUtils::getMacAddress();
            String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/status";
            JsonDocument resp;
            resp["status"] = "OK";

            String payloadStr;
            serializeJson(resp, payloadStr);

            publish(statusTopic.c_str(), payloadStr.c_str());
            Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
        }
    }
}

void MqttClient::mqttMessageCallback() {
    if(g_isConnected){
        String deviceId = DeviceUtils::getMacAddress();
        if(!g_userId.isEmpty()){
            mqttClient.setCallback([this, deviceId](char* topic, byte* payload, unsigned int length){
                String message;
                for (unsigned int i = 0; i < length; i++) {
                    message += (char)payload[i];
                }
    
                Serial.printf("[MQTT] Received on %s: %s\n", topic, message.c_str());
    
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);
                if (error) {
                    Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
                    return;
                }
    
                const char* msg = doc["command"];
                if (!msg) {
                    Serial.println("[MQTT] No command found in message.");
                    return;
                }
    
                String command = String(msg);
                
                if(command == "OTA_CHECK"){
                    Serial.println("[MQTT] OTA check requested");
                    OTAManager::handleOTACommand("CHECK_UPDATE");
                    
                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                    JsonDocument resp;
                    resp["status"] = "OK";
                    resp["message"] = "OTA check initiated";

                    String payloadStr;
                    serializeJson(resp, payloadStr);

                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                } else if(command == "OTA_UPDATE"){
                    const char* firmwareUrl = doc["firmwareUrl"];
                    if (firmwareUrl) {
                        Serial.printf("[MQTT] OTA update requested: %s\n", firmwareUrl);
                        OTAManager::handleOTACommand("FORCE_UPDATE", String(firmwareUrl));
                        
                        String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                        JsonDocument resp;
                        resp["status"] = "OK";
                        resp["message"] = "OTA update initiated";

                        String payloadStr;
                        serializeJson(resp, payloadStr);

                        publish(statusTopic.c_str(), payloadStr.c_str());
                        Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                    } else {
                        Serial.println("[MQTT] OTA update requested but no firmware URL provided");
                        
                        String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                        JsonDocument resp;
                        resp["status"] = "ERROR";
                        resp["message"] = "No firmware URL provided";

                        String payloadStr;
                        serializeJson(resp, payloadStr);

                        publish(statusTopic.c_str(), payloadStr.c_str());
                        Serial.printf("[MQTT] Published ERROR to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                    }
                } else {
                    Serial.printf("[MQTT] Unknown command: %s\n", command.c_str());
                }
            });
        }
    }
}

void MqttClient::otaSubscribe() {
    if(g_isConnected){
        String deviceId = DeviceUtils::getMacAddress();
        if(!g_userId.isEmpty()){
            String otaTopic = String("iot/") + g_userId + "/" + deviceId + "/ota";
            mqttClient.subscribe(otaTopic.c_str());
            Serial.printf("[MQTT] Subscribed to OTA topic: %s\n", otaTopic.c_str());
            
            mqttClient.setCallback([this, deviceId](char* topic, byte* payload, unsigned int length){
                String message;
                for (unsigned int i = 0; i < length; i++) {
                    message += (char)payload[i];
                }
    
                Serial.printf("[MQTT] Received OTA command on %s: %s\n", topic, message.c_str());
    
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);
                if (error) {
                    Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
                    return;
                }
    
                const char* action = doc["action"];
                if (!action) {
                    Serial.println("[MQTT] No action found in OTA message.");
                    return;
                }
    
                String otaAction = String(action);
                
                if(otaAction == "CHECK_UPDATE"){
                    Serial.println("[MQTT] OTA check requested via OTA topic");
                    OTAManager::handleOTACommand("CHECK_UPDATE");
                    
                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                    JsonDocument resp;
                    resp["status"] = "OK";
                    resp["message"] = "OTA check initiated";

                    String payloadStr;
                    serializeJson(resp, payloadStr);

                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                } else if(otaAction == "FORCE_UPDATE"){
                    const char* firmwareUrl = doc["firmwareUrl"];
                    if (firmwareUrl) {
                        Serial.printf("[MQTT] OTA update requested via OTA topic: %s\n", firmwareUrl);
                        OTAManager::handleOTACommand("FORCE_UPDATE", String(firmwareUrl));
                        
                        String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                        JsonDocument resp;
                        resp["status"] = "OK";
                        resp["message"] = "OTA update initiated";

                        String payloadStr;
                        serializeJson(resp, payloadStr);

                        publish(statusTopic.c_str(), payloadStr.c_str());
                        Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                    } else {
                        Serial.println("[MQTT] OTA update requested but no firmware URL provided");
                        
                        String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                        JsonDocument resp;
                        resp["status"] = "ERROR";
                        resp["message"] = "No firmware URL provided";

                        String payloadStr;
                        serializeJson(resp, payloadStr);

                        publish(statusTopic.c_str(), payloadStr.c_str());
                        Serial.printf("[MQTT] Published ERROR to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                    }
                } else {
                    Serial.printf("[MQTT] Unknown OTA action: %s\n", otaAction.c_str());
                }
            });
        }
    }
}

// New methods for continuous listening and power saving
void MqttClient::startContinuousListening() {
    isListeningMode = true;
    lastPublishTime = 0;
    Serial.println("[MQTT] Started continuous listening mode");
}

void MqttClient::stopContinuousListening() {
    isListeningMode = false;
    Serial.println("[MQTT] Stopped continuous listening mode - Power saving enabled");
}

void MqttClient::checkConnectionStatus() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastConnectionCheck >= connectionCheckInterval) {
        lastConnectionCheck = currentTime;
        
        // If we were connected but now disconnected, restart listening
        if (g_isConnected && !mqttClient.connected()) {
            Serial.println("[MQTT] Connection lost, restarting listening mode");
            g_isConnected = false;
            g_userId = "";
            startContinuousListening();
        }
        
        // If we're connected and in listening mode, stop it to save power
        if (g_isConnected && isListeningMode) {
            stopContinuousListening();
        }
    }
}

bool MqttClient::isInListeningMode() {
    return isListeningMode;
}
