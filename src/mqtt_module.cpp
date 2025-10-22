#include "include/mqtt_module.h"
#include "include/utils/json_transfer.h"
#include "include/utils/device_utils.h"
#include "include/utils/json_transfer.h"

bool g_isConnected = false;
String g_userId = "";
MqttClient* MqttClient::instance = nullptr;

MqttClient::MqttClient(const char* broker, int port, const char* clientId)
    : broker(broker), port(port), clientId(clientId), mqttClient(espClient), connected(false) {
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
        Serial.print("[MQTT] Connecting to broker...");
        if (mqttClient.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("connected!");
            connected = true;
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 2 seconds");
            delay(2000);
        }
    }
}

void MqttClient::loop() {
    mqttClient.loop();
}

void MqttClient::publish(const char* topic, const char* payload){
    if(mqttClient.connected()){
        mqttClient.publish(topic, payload);
        Serial.printf("[MQTT] Published to %s: %s\n", topic, payload);
    } else {
        Serial.printf("[MQTT] Not connected. Cannot publish!\n");
    }
}

void MqttClient::subscribe(const char* topic) {
    if(mqttClient.connected()){
        mqttClient.subscribe(topic);
        Serial.printf("[MQTT] Subscribed to topic: %s\n", topic);
    } else {
        Serial.printf("[MQTT] Not connected. Cannot subscribe!\n");
    }
}

void MqttClient::connectionPublish() {
    if(!g_isConnected) {
        String deviceId = DeviceUtils::getMacAddress();
        String topic = String("iot/") + deviceId + "/status";
        String payload = JsonTransferData::buildConnectPayload(deviceId.c_str(), "Connected");

        publish(topic.c_str(), payload.c_str());

    }
}

void MqttClient::connectionSubscribe() {
    if(!g_isConnected) {
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
                StaticJsonDocument<256> doc;
                DeserializationError error = deserializeJson(doc, message);
                if (error) {
                    Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
                    return;
                }

                const char* userId = doc["userId"];
                const char* msg = doc["message"];

                if (userId && msg && String(msg) == "Connected") {
                    g_userId = String(userId);
                    g_isConnected = true;

                    Serial.printf("[DEVICE] Connected confirmed by server. userId=%s\n", userId);

                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/status";
                    StaticJsonDocument<128> resp;
                    resp["status"] = "OK";

                    String payloadStr;
                    serializeJson(resp, payloadStr);

                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                }
            });
        } else {
            String deviceId = DeviceUtils::getMacAddress();
            String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/status";
            StaticJsonDocument<128> resp;
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
    
                StaticJsonDocument<256> doc;
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
                
                if(command == "LED_ON"){
                    DeviceUtils::handleLedOn();
                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                    StaticJsonDocument<128> resp;
                    resp["status"] = "OK";
    
                    String payloadStr;
                    serializeJson(resp, payloadStr);
    
                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                } else if(command == "LED_OFF"){
                    DeviceUtils::handleLedOff();
                    String statusTopic = String("iot/") + g_userId + "/" + deviceId + "/response";
                    StaticJsonDocument<128> resp;
                    resp["status"] = "OK";
    
                    String payloadStr;
                    serializeJson(resp, payloadStr);
    
                    publish(statusTopic.c_str(), payloadStr.c_str());
                    Serial.printf("[MQTT] Published OK to %s: %s\n", statusTopic.c_str(), payloadStr.c_str());
                } else {
                    Serial.printf("[MQTT] Unknown command: %s\n", command.c_str());
                }
            });
        }
    }
}
