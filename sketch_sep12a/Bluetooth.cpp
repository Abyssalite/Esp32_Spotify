#include "Bluetooth.h"

Bluetooth::Bluetooth()
    : _server(nullptr),
      _rxCharacteristic(nullptr),
      _txCharacteristic(nullptr) {}

void Bluetooth::begin()
{
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

void Bluetooth::send(const String& message) {
    if (_txCharacteristic == nullptr)
        return;

    _txCharacteristic->setValue(message);

    _txCharacteristic->notify();

    Serial.print("BLE TX: ");
    Serial.println(message);
}


Bluetooth::RxCallbacks::RxCallbacks(Bluetooth* bluetooth) : _bluetooth(bluetooth) {}

void Bluetooth::RxCallbacks::onWrite(
    NimBLECharacteristic* characteristic,
    NimBLEConnInfo& connInfo
){
    String value = characteristic->getValue().c_str();

    Serial.print("BLE RX: ");
    Serial.println(value);

    _bluetooth->send("ESP32 received: " + value);
}


Bluetooth::ServerCallbacks::ServerCallbacks(Bluetooth* bluetooth) : _bluetooth(bluetooth) {}

void Bluetooth::ServerCallbacks::onConnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo
){
    Serial.println("BLE client connected");
    Serial.printf("Info, reason=%d\n", connInfo);
}

void Bluetooth::ServerCallbacks::onDisconnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo,
    int reason
){
    Serial.printf("BLE client disconnected, reason=%d\n", reason);
    NimBLEDevice::startAdvertising();
}