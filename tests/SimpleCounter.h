#pragma once
#include "../BtnWatcher.h"
#include "SPIFFS.h"
auto dbgCntr = Dbg("[app]");
#define dbg dbgCntr

struct ProtocolClass {

  int cnt = 0;
  int minTimeToWaitMs = 2000;
  int pingTimeIntervalMs = minTimeToWaitMs;
  BtnWatcher btn;
  bool hasAck = false;
  bool waitingAck = false;
  ProtocolClass() {
    btn.onButton = [this](bool b) {
      dbg.print("button pressed", b);
      if (b)
        setMaster(!isMaster);
    };
  }

  bool getSavedMasterState() { return SPIFFS.exists("/isMaster.txt"); }

  void setMaster(bool _isMaster) {
    dbg.print("setting", (_isMaster ? "master" : "slave "));
    lastCnt = -1;
    dropped = 0;
    cnt = 0;
    isMaster = _isMaster;
    {
      auto sd = Display.getScope();
      Display.drawLine(isMaster ? "[Master] " : "[Slave]  ");
    }

    if (getSavedMasterState() != isMaster) {
      dbg.print("saving master state", isMaster ? "Mast" : "Slav");
      if (isMaster) {
        auto f = SPIFFS.open("/isMaster.txt", FILE_WRITE, true);
        if (!f) {
          Serial.println("There was an error opening the file for writing");
        }
        f.println("1");
        f.flush();
        f.close();
      } else {
        if (!SPIFFS.remove("/isMaster.txt")) {
          dbg.print("could not remove isMaster file");
        }
      }

      if (getSavedMasterState() != isMaster)
        dbg.print("!! setting master has not been saved");
    }
    if (isMaster) {
      LoraPhy.txMode();

      LoraPhy.onRxFlag = [this]() {
        dbg.print("new ack");
        waitingAck = false;
        // you can read received data as an Arduino String
        String str;
        LoraPhy.read(str);
        if (str != LoraPhy.lastMessageSent)
          dbg.print("err ack mismatch", str, LoraPhy.lastMessageSent);
        else
          hasAck = true;
        LoraPhy.txMode();
        updateDisp();
      };

      LoraPhy.onTxFlag = [this] {
        if (waitingAck && !hasAck) {
          dropped += 1;
        }
        hasAck = false;
        waitingAck = true;
        LoraPhy.rxMode();

        updateDisp();
      };
    } else { // if slave
      LoraPhy.rxMode();
      LoraPhy.onTxFlag = [this]() {
        // keep listening
        LoraPhy.rxMode();
      };

      LoraPhy.onRxFlag = [this] {
        // you can read received data as an Arduino String
        String str;
        LoraPhy.read(str);
        cnt = str.toInt();
        if (lastCnt < 0) {
          lastCnt = cnt - 1;
        } else if (cnt - lastCnt != 1) {
          dropped += 1;
        }
        lastCnt = cnt;
        updateDisp(str);
        delay(30);
        LoraPhy.send(str);
      };
    }
  }

  void begin() {
    LoraPhy.begin();
    setMaster(getSavedMasterState());
    int maxBytesToSend = 5;
    int maxMessageAirTime = LoraPhy.getTimeOnAirMs(maxBytesToSend);
    pingTimeIntervalMs = 2 * maxMessageAirTime + 200;
    if (pingTimeIntervalMs < minTimeToWaitMs)
      pingTimeIntervalMs = minTimeToWaitMs;
  }

  void end() { LoraPhy.end(); }

  void updateDisp(const String &str = "") {
    auto sd = Display.getScope();
    if (isMaster) {
      Display.drawLine("[Master] " + String(pingTimeIntervalMs / 1000.f));
      if (pingIntervalIsTooShort)
        Display.drawLine("ping interval too short");
      else
        Display.drawLine(getNiceCntStr());
      Display.drawLine("toa : " + String(LoraPhy.flagTxMs - sendTime));
      Display.drawLine("toa(est) : " + String(float(LoraPhy.getTimeOnAirMs(LoraPhy.lastSentPacketByteLen))));
      Display.drawLine("droppedAck : " + String(dropped));
    } else {
      Display.drawLine("[Slave] ");
      Display.drawLine(str);
      Display.drawLine(getNiceCntStr());
      Display.drawLine(String(dropped) + " dropped");
    }
  }
  void handle() {

    btn.handle();
    LoraPhy.handle();
    if (isMaster)
      handleMaster();
    else
      handleSlave();
  }

  void handleMaster() {

    // String bigStr = "";
    // for (int i = 0; i < 2 * cnt; i++) {
    //   bigStr += "1";
    // }
    auto now = millis();
    if (sendTime == 0 || (now - sendTime > pingTimeIntervalMs)) {
      cnt++;
      String cntStr(cnt, 10);
      sendTime = now;
      dbg.print("sending", cntStr);
      auto tToSend = LoraPhy.send(cntStr);
      auto minDelForAck = 2 * (tToSend / 1000);
      pingIntervalIsTooShort = minDelForAck > pingTimeIntervalMs;
    }
  }

  void handleSlave() {

    // nothing to do @see onFlag
  }

  String getNiceCntStr() const { return String("count : ") + String(cnt, 10); }
  unsigned long sendTime;
  bool isMaster;

  int lastCnt = -1;
  int dropped = 0;
  bool pingIntervalIsTooShort = false;
};

ProtocolClass protocol;

#undef dbg
