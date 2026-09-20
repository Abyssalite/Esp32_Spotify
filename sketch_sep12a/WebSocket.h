#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class WebSocket
{
public:
    using MessageHandler = void (*)(const String& message);
    using StatusHandler = void (*)(const String& status);

    WebSocket();
    void cleanClient();
    void begin();
    void notifyClients(const JsonDocument* json);
    void setMessageHandler(MessageHandler mHandler, StatusHandler sHandler);

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    MessageHandler _messageHandler;
    StatusHandler  _statusHandler;

    void onWsEvent(
        AsyncWebSocketClient* client,
        AwsEventType type,
        void* arg,
        uint8_t* data,
        size_t len
    );

    static void onWsEventStatic(
        AsyncWebSocket* server,
        AsyncWebSocketClient* client,
        AwsEventType type,
        void* arg,
        uint8_t* data,
        size_t len
    );

    static WebSocket* _instance;
};

#endif