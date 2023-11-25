#pragma once

#include "lib/MultiMDNS/MultiMDNS.h"

// TODO : WifiScan functionality adds a lot to binary size,it could be usefull to disable it
// TODO : WifiMulti does not check if a better rssi is available (happens when changing network conf)
#include <WiFi.h>
#include <WiFiMulti.h>

// custom wifi settings
#include <esp_wifi.h>

using std::string;
using std::vector;

auto dbgWifi = Dbg("[wifi]");
#define dbg dbgWifi

struct net {
  const char *ssid;
  const char *pass;
};
// if compile error here : you need to add your secrets as vector
#include "secrets.hpp"
net net0 = {"mange ma chatte", "sucemonbeat"};
// adds pass from secret networks...

//*********SSID and Pass for AP**************/
const char *ssidAP = "lumestrio";
const char *ssidAPPass = "sucemonbeat";

//*********Static IP Config**************/
IPAddress ap_local_IP(6, 6, 6, 1);
IPAddress ap_gateway(6, 6, 6, 1);
IPAddress ap_subnet(255, 255, 255, 0);
namespace MyWifi {

bool shouldDrawClients = true;
namespace conf {
unsigned short localWebPort = 80;
};

std::function<void(bool)> onWifiConnection;

// forward declare
bool sendPing();
void printEvent(WiFiEvent_t event);
void optimizeWiFi();
void WiFiAPEvent(WiFiEvent_t event);
void WiFiSTAEvent(WiFiEvent_t event);
bool ends_with(std::string const &value, std::string const &ending);
void drawClients();
// std::string getMac();

string instanceType;
std::string uid;
std::string instanceName;

bool connected = false;
bool lastConnectedStateHandled = false;
bool hasBeenDeconnected = false;
bool updateClients = false;
unsigned long long lastUpMDNS = 0;
std::string mdnsSrvTxt;

void connectToWiFiTask(void *params) {
  WiFiMulti wifiMulti;
  String addedNets = "";
  if (strlen(net0.ssid)) {
    wifiMulti.addAP(net0.ssid, net0.pass);
    addedNets += String(net0.ssid) + ", ";
  }
  for (auto &n : secrets::nets) {
    wifiMulti.addAP(n.ssid, n.pass);
    addedNets += String(n.ssid) + ", ";
  }
  dbg.print("scanning", addedNets);
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      if (connected) {
        connected = false;
        dbg.print("manually set  disconnected flag");
      }
      auto status = WiFi.status();
      unsigned long connectTimeout = 15000;
      if ((status == WL_CONNECT_FAILED) || (status == WL_CONNECTION_LOST) || (status == WL_DISCONNECTED) || (status == WL_NO_SHIELD) ||
          (status == WL_IDLE_STATUS)) {
        dbg.print("force try reconnect ", status);
        hasBeenDeconnected = false;
        // auto scanResult = WiFi.scanNetworks(true);
        wifiMulti.run(0);
        int timeOut = connectTimeout;
        constexpr int timeSlice = 1000;
        while ((timeOut > 0) && !hasBeenDeconnected) {
          vTaskDelay(timeSlice / portTICK_PERIOD_MS);
          timeOut -= timeSlice;
        }
        hasBeenDeconnected = false;
      }
      status = WiFi.status();

      if (status != WL_CONNECTED) {
        dbg.print("no connected", status);
      }
      /*
       */
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

bool getSavedAPMode() { return SPIFFS.exists("/isAP.txt"); }

void setAPMode(bool b) {
  if (b != getSavedAPMode()) {
    dbg.print("saving apmode", b ? "AP" : "STA");
    if (b) {
      auto f = SPIFFS.open("/isAP.txt", FILE_WRITE, true);
      if (!f) {
        Serial.println("There was an error opening the file for writing");
      }
      f.println("1");
      f.flush();
      f.close();
    } else {
      if (!SPIFFS.remove("/isAP.txt")) {
        dbg.print("could not remove isAP file");
      }
    }
    // reboot for now
    delay(100);
    ESP.restart();
  }
}

bool APMode = true;
void begin(const std::string &type, const std::string &_uid) {
  APMode = getSavedAPMode();
  instanceType = type;
  uid = _uid;

  // delay(1000);
  dbg.print("setting up : ", uid.c_str());

  mdnsSrvTxt = instanceType + "@" + getMac();
  // delay(100);
  instanceName = instanceType;
  if (uid.size()) {

    if ((uid.rfind(instanceName + "_", 0) == 0) && uid.size() > instanceName.size() + 1) {
      instanceName = uid;
    } else {
      instanceName += "_" + uid;
    }
    string localSuf = ".local";
    if (ends_with(instanceName, localSuf)) {
      instanceName = instanceName.substr(0, instanceName.length() - localSuf.length());
    }
    string forcedSuf = ".esp";
    if (!ends_with(instanceName, forcedSuf)) {
      instanceName += forcedSuf;
    }
  }

  // register event handler

  if (APMode) {
    WiFi.onEvent(WiFiAPEvent);
    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    // constexpr int DNS_PORT = 53;
    // dnsServer.start(DNS_PORT, "*", apIP);
    WiFi.mode(WIFI_AP);
    WiFi.softAPsetHostname(instanceName.c_str());
    connected = WiFi.softAP(ssidAP, ssidAPPass, 1, false, 10);
    Serial.println(connected ? "soft-AP setup" : "Failed to connect");
    delay(100);
    Serial.println(WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet) ? "Configuring Soft AP" : "Error in Configuration");
    Serial.println(WiFi.softAPIP());

  } else { // here we could setSTA+AP if needed (supported by wifiMulti normally)
    WiFi.setHostname(instanceName.c_str());
    WiFi.onEvent(WiFiSTAEvent);
    int stackSz = 5000;
    xTaskCreatePinnedToCore(connectToWiFiTask, "keepwifi", stackSz, NULL, 1, NULL, (CONFIG_ARDUINO_RUNNING_CORE + 1) % 2);
  }
}

bool handle() {
  if (lastConnectedStateHandled != connected) {
    if (onWifiConnection)
      onWifiConnection(connected);
    lastConnectedStateHandled = connected;
  }
  // if (connected && shouldDrawClients && (millis() - lastUpMDNS > 15000)) {
  //   updateClients = true;
  // }
  if (connected && updateClients) {

    drawClients();
    updateClients = false;
  }

  return connected;
}

/// string helps
bool ends_with(std::string const &value, std::string const &ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

String joinToString(vector<float> p) {
  String res;
  for (auto &e : p) {
    res += ", " + String(e);
  }
  return res;
}

std::vector<String> getConnectedClients() {
  if (!APMode)
    return {};
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;

  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

  std::vector<String> res;
  for (int i = 0; i < adapter_sta_list.num; i++) {

    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];

    // Serial.print("station nr ");
    // Serial.println(i);

    // Serial.print("MAC: ");

    // for (int i = 0; i < 6; i++) {

    //   Serial.printf("%02X", station.mac[i]);
    //   if (i < 5)
    //     Serial.print(":");
    // }

    // Serial.print("\nIP: ");
    // Serial.println();
    ip4_addr_t addr = {station.ip.addr};
    res.push_back(String(ip4addr_ntoa(&(addr))));
  }
  return res;
}

struct CapT {
  String name;
  String url;
};
struct RegClient {
  RegClient(const String &n) : name(n) {}
  String name;
  std::vector<CapT> caps;
};

std::vector<RegClient> regClients;

void addRegClient(const String &hn, const String &ip) {
  RegClient rc(hn);
  // dbg.print("addingReg", hn, "::", ip);
  if (hn.startsWith("lumestrio")) {
    rc.caps.push_back({"vermuth", ip + ":3005"});
    rc.caps.push_back({"son", ip + ":8000"});
  }
  regClients.push_back(rc);
}

void drawClients() {
  if (!shouldDrawClients)
    return;

  auto lk = DisplayScope::get();
  Display.drawLine("clients");
  auto con = getConnectedClients();
  int numServices = con.size() ? MultiMDNS.queryService("rspstrio", "udp") : 0;
  lastUpMDNS = millis();
  regClients.clear();
  for (const auto &c : con) {
    String host = "";
    dbg.print(c);
    if (c.length() && (c != "0.0.0.0")) {
      for (int i = 0; i < numServices; i++) {
        auto hn = MultiMDNS.hostname(i);
        String ipStr = MultiMDNS.IP(i, ap_local_IP).toString();
        if (ipStr == c) {
          host = hn;
          addRegClient(hn, ipStr);
          break;
        }
      }
    }
    Display.drawLine(c + " " + host);
  }
  String toReboot;
  dbg.print("------");
  for (int i = 0; i < numServices; i++) {
    auto hn = MultiMDNS.hostname(i);
    String ipStr = MultiMDNS.IP(i, ap_local_IP).toString();
    dbg.print(i, hn, ipStr);
    if (ipStr != "0.0.0.0" && !ipStr.startsWith("6.6.6."))
      toReboot += hn + " ";
  }
  if (toReboot.length())
    Display.drawLine("reboot " + toReboot);
}
// wifi event handler
void WiFiAPEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_START:
    updateClients = true;
    optimizeWiFi();
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STOP:
    // dbg.print("WiFi access point  stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    // dbg.print("Client connected");
    // updateClients = true;
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    // dbg.print("Client disconnected");
    updateClients = true;
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    updateClients = true;
    dbg.print("Assigned IP address to client");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    dbg.print("Received probe request");
    break;
  }
}

void WiFiSTAEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_GOT_IP:
    optimizeWiFi();

    // When connected set
    dbg.print("WiFi connected to ", WiFi.SSID(), WiFi.getHostname(), " @ ", WiFi.localIP());
    {
      auto lk = DisplayScope::get();
      Display.drawLine(String("WiFi connected to ") + WiFi.SSID());
      Display.drawLine(String(WiFi.getHostname()) + " @ " + WiFi.localIP());
    }

    dbg.print("announce mdns", instanceName);
    MultiMDNS.begin(instanceName.c_str());
    MultiMDNS.addService("http", "tcp", conf::localWebPort);
    // MultiMDNS.addServiceTxt("rspstrio", "udp", "uuid", mdnsSrvTxt.c_str());
    // digitalWrite(ledPin, HIGH);
    connected = true;

    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    dbg.print("WiFi lost connection");
    hasBeenDeconnected = true;
    connected = false;
    break;
  default:
    // dbg.print("Wifi Event :",event);
    break;
  }
}
void optimizeWiFi() {
  return;
  if (WiFi.status() == WL_CONNECTED) {
    dbg.print("optimizing wifi connection");
    // no power saving here
    if (auto err = esp_wifi_set_max_tx_power(82)) {
      dbg.print("can't increase power : ");
      switch (err) {
      //: WiFi is not initialized by esp_wifi_init
      case (ESP_ERR_WIFI_NOT_INIT):
        dbg.print("wifi not inited");
      //: WiFi is not started by esp_wifi_start
      case (ESP_ERR_WIFI_NOT_STARTED):
        dbg.print("wifi not started");
      //: invalid argument, e.g. parameter is out of range
      // case (ESP_ERR_WIFI_ARG):
      //   dbg.print("wrong args");
      default:
        dbg.print("unknown err");
      }
    }

    // we do not want to be sleeping !!!!
    // ButBluetooth
    if (!WiFi.setSleep(wifi_ps_type_t::WIFI_PS_MIN_MODEM)) {
      dbg.print("can't stop sleep wifi");
    }
  } else {
    dbg.print("can't optimize, not connected");
  }
}

void printEvent(WiFiEvent_t event) {
  switch (event) {
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_READY:
    dbg.print("WiFi interface ready");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_SCAN_DONE:
    dbg.print("Completed scan for access points");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_START:
    dbg.print("WiFi client started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_STOP:
    dbg.print("WiFi clients stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_CONNECTED:
    dbg.print("Connected to access point");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    dbg.print("Disconnected from WiFi access point");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    dbg.print("Authentication mode of access point has changed");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_GOT_IP:
    dbg.print("Obtained IP address: ", WiFi.localIP());
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_STA_LOST_IP:
    dbg.print("Lost IP address and IP address is reset to 0");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_SUCCESS:
    dbg.print("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_FAILED:
    dbg.print("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_TIMEOUT:
    dbg.print("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WPS_ER_PIN:
    dbg.print("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_START:
    dbg.print("WiFi access point started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STOP:
    dbg.print("WiFi access point  stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    dbg.print("Client connected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    dbg.print("Client disconnected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    dbg.print("Assigned IP address to client");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    dbg.print("Received probe request");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    dbg.print("IPv6 is preferred");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_START:
    dbg.print("Ethernet started");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_STOP:
    dbg.print("Ethernet stopped");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_CONNECTED:
    dbg.print("Ethernet connected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_DISCONNECTED:
    dbg.print("Ethernet disconnected");
    break;
  case arduino_event_id_t::ARDUINO_EVENT_ETH_GOT_IP:
    dbg.print("Obtained IP address");
    break;
  default:
    dbg.print("unknown", event);
    break;
  }
}
} // namespace MyWifi

#undef dbg
