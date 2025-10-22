#pragma once
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "include/secrets.h"

extern bool g_isConnected;
extern String g_userId;

class MqttClient {
    private:
        static MqttClient* instance;
        WiFiClientSecure espClient;
        PubSubClient mqttClient;
        const char* broker;
        int port;
        const char* clientId;
        bool connected;

        MqttClient(const char* broker, int port, const char* clientId);
        
    public: 
        MqttClient(const MqttClient&) = delete;
        MqttClient& operator = (const MqttClient&) = delete;

        static MqttClient* getInstance(
            const char* broker = MQTT_BROKER_URL,
            int port = MQTT_PORT,
            const char* clientId = MQTT_CLIENT_ID
        );

    void connect();

    void loop();

    void publish(const char* topic, const char* payload);

    void subscribe(const char* topic);

    void connectionPublish();

    void connectionSubscribe();

    void mqttMessageCallback();
};

