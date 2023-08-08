#include "boardDef.h"

#define BLE_SRV 0
#define WIFI_SRV 1
/// local
#include "Dbg.h"

#include "Display.h"

// #include "LoraPhyRadio.h"
#include "LoraPhyE32.h"
#include "tests/SimpleCounter.h"
#include "uuid.h"
#if BLE_SRV
#include "MyBLEServer.h"
#endif

auto dbg = Dbg("[main]");

int counter = 0;

#if WIFI_SRV
#include "wifi.h"
#include "ESPWebFs/WebFS.h"
AsyncWebServer webServer(80);
#endif

void setup() {
  Serial.begin(115200);
  board::init();
  delay(500);
  // When the power is turned on, a delay is required.
  if (!SPIFFS.begin(true)) {
    dbg.print("spiff error in init()", esp_err_to_name(258));
  }
  delay(500);
  dbg.print("start");

  protocol.begin(); // has to call LoraPhy.begin();

#if WIFI_SRV
  // Start webServer
  MyWifi::begin("test", "");
  MyWifi::onWifiConnection = [](bool isConnected) {
    if (isConnected) {
      webServer.begin();
      WebFS::setup(webServer, String(getMac().c_str()), true);
    }
  };
#endif
#if BLE_SRV
  MyBLEServer::begin();
#endif
}

void loop() {
  board::loop();
#if WIFI_SRV
  WebFS::loop();
  if (WebFS::isDoingOTA()) {
    auto lk = DisplayScope::get();
    int pctDone = 0;
    if (Update.size() != 0)
      pctDone = 100.f * Update.progress() / Update.size();
    Display.drawLine("updating : " + String(pctDone));
    return;
  }
#endif
  // LoraPhy.handle();
  protocol.handle();
#if BLE_SRV
  MyBLEServer::handle();
#endif
#if WIFI_SRV
  MyWifi::handle();
#endif
  // delay(100);
}

/*
void sendCountPkt() {
  Serial.print("Sending packet: ");
  Serial.println(counter);
  unsigned long long tbeforeSend = millis();
  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();
  auto deltaSend = millis() - tbeforeSend;

  Display.begin();
  char buf[256];
  snprintf(buf, sizeof(buf), "SF: %d ,bw: %d", LoRa.getSpreadingFactor(), LoRa.getSignalBandwidth());
  Display.drawLine(buf);
  Display.drawLine("Transmitting: OK!");
  snprintf(buf, sizeof(buf), "Sending: %d (%d)", counter, deltaSend);
  Display.drawLine(buf);
  Display.end();
  counter++;
}
*/
