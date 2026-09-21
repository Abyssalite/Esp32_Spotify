#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>

#include "env.h"

class Bluetooth {
  public:
    using MessageHandler = void (*)(const String& message);
    using StatusHandler = void (*)(const int& status);

    Bluetooth();

    void begin();
    void stop();
    void send(const String& message);
    void setMessageHandler(MessageHandler mHandler, StatusHandler sHandler);

  private:
    NimBLEServer* _server;
    NimBLECharacteristic* _rxCharacteristic;
    NimBLECharacteristic* _txCharacteristic;
    MessageHandler _messageHandler;
    StatusHandler  _statusHandler;

    class RxCallbacks : public NimBLECharacteristicCallbacks {
      public:
        explicit RxCallbacks(Bluetooth* bluetooth);

        void onWrite(
          NimBLECharacteristic* characteristic,
          NimBLEConnInfo& connInfo
        ) override;

      private:
        Bluetooth* _bluetooth;
    };

    class ServerCallbacks : public NimBLEServerCallbacks{
      public:
        explicit ServerCallbacks(Bluetooth* bluetooth);

        void onConnect(
          NimBLEServer* server,
          NimBLEConnInfo& connInfo
        ) override;

        void onDisconnect(
          NimBLEServer* server,
          NimBLEConnInfo& connInfo,
          int reason
        ) override;

      private:
        Bluetooth* _bluetooth;
    };
};

#endif