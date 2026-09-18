#include "WebSocket.h"

WebSocket* WebSocket::_instance = nullptr;

WebSocket::WebSocket()
    : _server(80),
      _ws("/ws"),
      _status(""),
      _messageHandler(nullptr),
      _statusHandler(nullptr)
{
    _instance = this;
}

void WebSocket::begin()
{
    _ws.onEvent(WebSocket::onWsEventStatic);

    _server.addHandler(&_ws);
    _server.begin();

    Serial.println("WS started");
}

void WebSocket::setMessageHandler(MessageHandler mHandler, StatusHandler sHandler) {
    _messageHandler = mHandler;
    _statusHandler  = sHandler;
}

void WebSocket::notifyClients(const JsonDocument* json) {
    if (json == nullptr)
        return;

    String jsonString;
    serializeJson(*json, jsonString);

    _ws.textAll(jsonString);
    _ws.cleanupClients();
}

void WebSocket::onWsEventStatic(
    AsyncWebSocket* server,
    AsyncWebSocketClient* client,
    AwsEventType type,
    void* arg,
    uint8_t* data,
    size_t len
){
    if (_instance == nullptr)
        return;

    _instance->onWsEvent(client, type, arg, data, len);
}

void WebSocket::onWsEvent(
    AsyncWebSocketClient* client,
    AwsEventType type,
    void* arg,
    uint8_t* data,
    size_t len
) {
    if (type == WS_EVT_CONNECT){
        _status = "WS connected";
    }
    else if (type == WS_EVT_DISCONNECT){
        _status = "WS disconnect";
        _ws.cleanupClients();
    }
    else if (type == WS_EVT_DATA){
        _message = (reinterpret_cast<char*>(data), len);
    }

    if (_messageHandler != nullptr) {
        _messageHandler(_message);
    }
    if (_statusHandler != nullptr) {
        _statusHandler(_status);
    }
}