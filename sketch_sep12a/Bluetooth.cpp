#include "Bluetooth.h"

Bluetooth::Bluetooth()
    : _server(nullptr),
      _rxCharacteristic(nullptr),
      _txCharacteristic(nullptr),
      _messageHandler(nullptr),
      _statusHandler(nullptr) {}

void Bluetooth::begin() {
    NimBLEDevice::init("ESP32-Control");

    _server = NimBLEDevice::createServer();
    _server->setCallbacks(new ServerCallbacks(this));

    NimBLEService* service = _server->createService(SERVICE_UUID);

    _rxCharacteristic = service->createCharacteristic(
            RX_CHARACTERISTIC,
            NIMBLE_PROPERTY::WRITE
        );

    _txCharacteristic = service->createCharacteristic(
            TX_CHARACTERISTIC,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY
        );

    _rxCharacteristic->setCallbacks(new RxCallbacks(this));

    service->start();
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();

    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setName("ESP32-Control");
    advertising->start();

    Serial.println("BLE advertising started");
}

void Bluetooth::stop() {
    Serial.println("Stopping BLE...");
    _server = nullptr;
    _rxCharacteristic = nullptr;
    _txCharacteristic = nullptr;

    NimBLEDevice::deinit(true);

    Serial.printf("Free heap after BLE shutdown: %u\n", ESP.getFreeHeap());
}

void Bluetooth::setMessageHandler(MessageHandler mHandler, StatusHandler sHandler) {
    _messageHandler = mHandler;
    _statusHandler  = sHandler;
}

void Bluetooth::send(const String& message) {
    if (_txCharacteristic == nullptr)
        return;

    _txCharacteristic->setValue(message);

    _txCharacteristic->notify();
}


Bluetooth::RxCallbacks::RxCallbacks(Bluetooth* bluetooth) : _bluetooth(bluetooth) {}

void Bluetooth::RxCallbacks::onWrite(
    NimBLECharacteristic* characteristic,
    NimBLEConnInfo& connInfo
){
    String value = characteristic->getValue().c_str();

    if (_bluetooth->_messageHandler != nullptr)
    {
        _bluetooth->_messageHandler(value);
    }
}


Bluetooth::ServerCallbacks::ServerCallbacks(Bluetooth* bluetooth) : _bluetooth(bluetooth) {}

void Bluetooth::ServerCallbacks::onConnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo
){
    if (_bluetooth->_statusHandler != nullptr)
    {
       _bluetooth->_statusHandler(1);
    }
}

void Bluetooth::ServerCallbacks::onDisconnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo,
    int reason
){
    if (_bluetooth->_statusHandler != nullptr)
    {
        _bluetooth->_statusHandler(reason);
    }
    NimBLEDevice::startAdvertising();
}