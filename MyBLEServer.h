
/** NimBLE_Server Demo:
 *
 *  Demonstrates many of the available features of the NimBLE server library.
 *
 *  Created: on March 22 2020
 *      Author: H2zero
 *
 */

#include <NimBLEDevice.h>
#include "uuid.h"

auto dbgBle = Dbg("[ble]");
#define dbg dbgBle

namespace MyBLEServer {

const char *BT_SERVICE_UUID = "74356090-d5dc-11ed-b16f-0800200c9a66";
const char *BT_TORADIO_UUID = "7b49d0f0-d5dc-11ed-b16f-0800200c9a66";
const char *BT_FROMRADIO_UUID = "82f2e030-d5dc-11ed-b16f-0800200c9a66";
const uint32_t BT_PASSKEY = 123456;

unsigned long lastNotifSend = 0;
int notifIntervalMs = 5000;

static NimBLEServer *bleServer = nullptr;

class ServerCallback : public NimBLEServerCallbacks {
  uint32_t onPassKeyRequest() override {
    uint32_t passkey = BT_PASSKEY; // config.bluetooth.fixed_pin;
    // LOG_INFO("*** Enter passkey %d on the peer side ***\n", passkey);
    return passkey;
  }

  void onAuthenticationComplete(ble_gap_conn_desc *desc) override { dbg.print("BLE authentication complete"); }

  void onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override { dbg.print("BLE disconnect"); }
};

class ToRadioCallbackClass : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic) override {
    dbg.print("To Radio onwrite");
    auto val = pCharacteristic->getValue();
    // do anything with it
  }
};

class FromRadioCallbackClass : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic *pCharacteristic) override {
    dbg.print("From Radio onread");
    // uint8_t fromRadioBytes[meshtastic_FromRadio_size];
    // todo unpack
    // size_t numBytes = 0;
    // bluetoothPhoneAPI->getFromRadio(fromRadioBytes);

    // std::string fromRadioByteString(fromRadioBytes, fromRadioBytes + numBytes);

    // pCharacteristic->setValue(fromRadioByteString);
  }
};

void setupService() {

  NimBLEService *bleService = bleServer->createService(BT_SERVICE_UUID);
  NimBLECharacteristic *ToRadioCharacteristic;
  NimBLECharacteristic *FromRadioCharacteristic;
  ToRadioCharacteristic = bleService->createCharacteristic(BT_TORADIO_UUID, NIMBLE_PROPERTY::WRITE);
  FromRadioCharacteristic = bleService->createCharacteristic(BT_FROMRADIO_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  auto *toRadioCallbacks = new ToRadioCallbackClass();
  ToRadioCharacteristic->setCallbacks(toRadioCallbacks);

  auto *fromRadioCallbacks = new FromRadioCallbackClass();
  FromRadioCharacteristic->setCallbacks(fromRadioCallbacks);

  bleService->start();
}

void startAdvertising() {
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->reset();
  // pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

void begin() {
  dbg.print("Starting NimBLE Server");
  /** sets device name */
  auto deviceName = "bt-" + getMac();
  NimBLEDevice::init(deviceName);
  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
  /** Set the IO capabilities of the device, each option will trigger a different pairing method.
   *  BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
   *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
   *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
   */
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
  /** 2 different ways to set security - both calls achieve the same result.
   *  no bonding, no man in the middle protection, secure connections.
   *
   *  These are the default values, only shown here for demonstration.
   */
  // NimBLEDevice::setSecurityAuth(false, false, true);
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

  // Server
  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallback(), true);
  setupService();
  startAdvertising();
  dbg.print("Advertising Started for", deviceName, "full name", NimBLEDevice::toString());
}

void handle() {
  auto now = millis();
  if (now - lastNotifSend < (unsigned long)(notifIntervalMs))
    return;

  lastNotifSend = now;
  dbg.print("try send notif to", bleServer->getConnectedCount());
  /** Do your thing here, this just spams notifications to all connected clients */
  if (bleServer->getConnectedCount()) {
    NimBLEService *pSvc = bleServer->getServiceByUUID(BT_SERVICE_UUID);
    if (pSvc) {
      NimBLECharacteristic *pChr = pSvc->getCharacteristic(BT_FROMRADIO_UUID);
      if (pChr) {
        dbg.print("found chr to send");
        pChr->notify(true);
      }
    }
  }
}
}; // namespace MyBLEServer
#undef dbg
