#include "boardDef.h"

#if PROTO_DUPONT_DEVKIT
#define BLE_SRV 0
#define WIFI_SRV 0
#define HAS_LORA 0
#else
#define BLE_SRV 0
#define WIFI_SRV 1
#if ARDUINO_M5Stack_Core_ESP32
#define HAS_LORA 1
#else
#define HAS_LORA 0
#endif
#endif
/// local
#include "Dbg.h"

#include "Display.h"

#if HAS_LORA
// #include "LoraPhyRadio.h"
#include "MainProtocol.h"
#endif
#include "uuid.h"
#if BLE_SRV
#include "MyBLEServer.h"
#endif

auto dbg = Dbg("[main]");

int counter = 0;

int drawTypeIdx = 0;
std::vector<String> drawTypes = {"A", "W", "L"};
#if WIFI_SRV
#include "wifi.h"
#include "ESPWebFs/WebFS.h"
#include <DNSServer.h>
AsyncWebServer webServer(80);
// DNSServer dnsServer;
#endif

void updateHeaderInfos() {
  if (MyWifi::APMode)
    Display.headerInfos = "AP ";
  else
    Display.headerInfos = "STA ";
  if (MyWifi::connected) {
    Display.headerInfos += WiFi.SSID();
  } else {
    Display.headerInfos += " Offline";
  }
  Display.headerInfos += " " + drawTypes[drawTypeIdx];
}
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

#if HAS_LORA
  protocol.begin(); // has to call LoraPhy.begin();
#endif
#if WIFI_SRV
  // Start webServer
  MyWifi::begin("test", "");

  MyWifi::onWifiConnection = [](bool isConnected) {
    updateHeaderInfos();
    if (isConnected) {
      webServer.begin();
      WebFS::setup(webServer, String(getMac().c_str()), true);
      webServer.on("/devices", HTTP_GET, [](AsyncWebServerRequest *request) {
        MyWifi::drawClients();
        String res = R"rawliteral(<!DOCTYPE HTML>\n<html><body>
<style>
    .dev{
    display : flex;
    justify-content: space-around;
    min-height: 50px;
    border-style: solid;
    }
</style>)rawliteral";

        res += "<a href=setOnOff?d=all&v=1>all on</a>";
        res += "<a href=setOnOff?d=all&v=0>all off</a>";
        for (const auto &c : MyWifi::regClients) {
          res += "<div class=\"dev\">";
          res += c.name;
          for (const auto &cap : c.caps) {
            res += "<a target=\"_blank\" href=\"http://" + cap.url + "\">";
            res += cap.name;
            res += "</a>";
          }

          res += "<a href=setOnOff?d=" + c.name + "&v=1>on</a>";
          res += "<a href=setOnOff?d=" + c.name + "&v=0>off</a>";
          res += "</div>";
        }
        res += "</body></html>";
        request->send(200, "text/html", res);
      });
    }
    webServer.on("/setOnOff", HTTP_GET, [](AsyncWebServerRequest *request) {
      String device = request->getParam("d")->value();
      String isOn = request->getParam("v")->value();
      uint8_t active = isOn == "1" ? 1 : 0;
      int num = 0;
      int t = -1;
      if (device.startsWith("lumestrio")) {
        t = 1;
        device.replace("lumestrio", "");
        num = device.toInt();
      } else if (device.startsWith("relay_")) {
        t = 0;
        device.replace("relay_", "");
        num = device.toInt();
      } else if (device == "all") {
        t = 0;
        num = 255;
      }
      uint8_t loraUuid = 32 * t + num;
      dbg.print("will send act", device, isOn, loraUuid);
      uint8_t actId = 4; // MESSAGE_TYPE::ACTIVATE
      std::vector<uint8_t> actMsg = {actId, active, loraUuid};
      LoraPhy.send(actMsg.data(), actMsg.size());
      request->send(200, "text/plain", "ok");
    });
  };
#endif
#if BLE_SRV
  MyBLEServer::begin();
#endif
}

int lastUpdateProg = 0;
void loop() {
  board::loop();
#if WIFI_SRV
  WebFS::loop();
  if (WebFS::isDoingOTA()) {
    int pctDone = 0;
    if (Update.size() != 0)
      pctDone = 100.f * Update.progress() / Update.size();
    if (lastUpdateProg != int(pctDone)) {
      auto lk = DisplayScope::get();
      lastUpdateProg = int(pctDone);
      Display.drawLine("updating : " + String(pctDone));
    }
    return;
  }
#endif

#if HAS_LORA
  protocol.handle();
#endif

#if BLE_SRV
  MyBLEServer::handle();
#endif
#if WIFI_SRV
  MyWifi::handle();
#endif

#if M5STACK_CORE1 || ARDUINO_M5STACK_Core2
  if (M5.BtnC.wasPressed()) {
    Display.drawDick();
  }

  if (M5.BtnB.pressedFor(2000)) {
    MyWifi::setAPMode(!MyWifi::getSavedAPMode());
  } else if (M5.BtnB.wasPressed()) {
    MyWifi::drawClients();
  }

  if (M5.BtnA.wasPressed()) {
    drawTypeIdx++;
    drawTypeIdx = drawTypeIdx % drawTypes.size();

    shouldDisplayLoraMsg = drawTypeIdx == 0 || drawTypeIdx == 2;
    LoraPhy.ignoreLora = !shouldDisplayLoraMsg;
    MyWifi::shouldDrawClients = drawTypeIdx == 0 || drawTypeIdx == 1;
    if (MyWifi::shouldDrawClients)
      MyWifi::updateClients = true;
    updateHeaderInfos();
    auto lk = DisplayScope::get();

    Display.drawLine(String("switch draw mode ") + drawTypes[drawTypeIdx]);
  }

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
