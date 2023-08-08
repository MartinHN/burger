#pragma once

#include "ESPmDNS.h"

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

namespace MyWifi {

namespace conf {
unsigned short localWebPort = 80;
};

std::function<void(bool)> onWifiConnection;

// forward declare
bool sendPing();
void printEvent(WiFiEvent_t event);
void optimizeWiFi();
// std::string getMac();

string instanceType;
std::string uid;
std::string instanceName;

bool connected = false;
bool lastConnectedStateHandled = false;
bool hasBeenDeconnected = false;

std::string mdnsSrvTxt;
// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  printEvent(event);
  switch (event) {
  case SYSTEM_EVENT_STA_GOT_IP:

    optimizeWiFi();

    // When connected set
    dbg.print("WiFi connected to ", WiFi.SSID(), WiFi.getHostname(), " @ ", WiFi.localIP());
    {
      auto lk = DisplayScope::get();
      Display.drawLine(String("WiFi connected to ") + WiFi.SSID());
      Display.drawLine(String(WiFi.getHostname()) + " @ " + WiFi.localIP());
    }

    dbg.print("announce mdns", instanceName);
    MDNS.begin(instanceName.c_str());
    MDNS.addService("http", "tcp", conf::localWebPort);
    // MDNS.addServiceTxt("rspstrio", "udp", "uuid", mdnsSrvTxt.c_str());
    // digitalWrite(ledPin, HIGH);
    connected = true;

    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
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

inline bool ends_with(std::string const &value, std::string const &ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void begin(const std::string &type, const std::string &_uid) {
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

  WiFi.setHostname(instanceName.c_str());
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // here we could setSTA+AP if needed (supported by wifiMulti normally)
  int stackSz = 5000;
  xTaskCreatePinnedToCore(connectToWiFiTask, "keepwifi", stackSz, NULL, 1, NULL, (CONFIG_ARDUINO_RUNNING_CORE + 1) % 2);
}

bool handle() {
  if (lastConnectedStateHandled != connected) {
    if (onWifiConnection)
      onWifiConnection(connected);
    lastConnectedStateHandled = connected;
  }
  return connected;
}

String joinToString(vector<float> p) {
  String res;
  for (auto &e : p) {
    res += ", " + String(e);
  }
  return res;
}

void printEvent(WiFiEvent_t event) {
  switch (event) {
  case SYSTEM_EVENT_WIFI_READY:
    dbg.print("WiFi interface ready");
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    dbg.print("Completed scan for access points");
    break;
  case SYSTEM_EVENT_STA_START:
    dbg.print("WiFi client started");
    break;
  case SYSTEM_EVENT_STA_STOP:
    dbg.print("WiFi clients stopped");
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    dbg.print("Connected to access point");
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    dbg.print("Disconnected from WiFi access point");
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    dbg.print("Authentication mode of access point has changed");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    dbg.print("Obtained IP address: ", WiFi.localIP());
    break;
  case SYSTEM_EVENT_STA_LOST_IP:
    dbg.print("Lost IP address and IP address is reset to 0");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    dbg.print("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    dbg.print("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    dbg.print("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
    dbg.print("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case SYSTEM_EVENT_AP_START:
    dbg.print("WiFi access point started");
    break;
  case SYSTEM_EVENT_AP_STOP:
    dbg.print("WiFi access point  stopped");
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    dbg.print("Client connected");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    dbg.print("Client disconnected");
    break;
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
    dbg.print("Assigned IP address to client");
    break;
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
    dbg.print("Received probe request");
    break;
  case SYSTEM_EVENT_GOT_IP6:
    dbg.print("IPv6 is preferred");
    break;
  case SYSTEM_EVENT_ETH_START:
    dbg.print("Ethernet started");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    dbg.print("Ethernet stopped");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    dbg.print("Ethernet connected");
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    dbg.print("Ethernet disconnected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    dbg.print("Obtained IP address");
    break;
  default:
    dbg.print("unknown");
    break;
  }
}
} // namespace MyWifi

#undef dbg
