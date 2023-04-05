#pragma once
#include "../BtnWatcher.h"
auto dbgCntr = Dbg("[cntr]");
#define dbg dbgCntr

struct ProtocolClass {

  int cnt = 100;

  BtnWatcher btn;
  ProtocolClass() {
    btn.onButton = [this](bool b) {
      if (b)
        setMaster(!isMaster);
    };
  }
  void setMaster(bool _isMaster) {
    dbg.print("setting", (_isMaster ? "master" : "slave "));
    isMaster = _isMaster;
    {
      auto sd = Display.getScope();
      Display.drawLine(isMaster ? "[Master] " : "[Slave]  ");
    }
    if (isMaster) {
      LoraPhy.txMode();
      LoraPhy.onFlag = [this] {
        auto sd = Display.getScope();
        Display.drawLine("[Master] ");
        Display.drawLine(getNiceCntStr());
        Display.drawLine("toa : " + String(LoraPhy.flagMs - sendTime));
        Display.drawLine("toa(est) : " + String(float(LoraPhy.radio.getTimeOnAir(LoraPhy.lastSentPacketByteLen) / 1000)));
      };
    } else {
      LoraPhy.rxMode();
      LoraPhy.onFlag = [this] {
        // you can read received data as an Arduino String
        String str;
        LoraPhy.read(str);
        cnt = str.toInt();
        {
          auto sd = Display.getScope();
          Display.drawLine("[Slave] ");
          Display.drawLine(str);
          Display.drawLine(getNiceCntStr());
        }
        // keep listening
        LoraPhy.rxMode();
      };
    }
  }

  void begin() {
    LoraPhy.begin();
    setMaster(false);
  }

  void end() { LoraPhy.end(); }

  void handle() {

    btn.handle();
    LoraPhy.handle();
    if (isMaster)
      handleMaster();
    else
      handleSlave();
  }

  void handleMaster() {
    String cntStr(cnt++, 10);
    // String bigStr = "";
    // for (int i = 0; i < 2 * cnt; i++) {
    //   bigStr += "1";
    // }
    sendTime = millis();
    auto tToSend = LoraPhy.send(cntStr);
    auto del = max<float>(300, tToSend / 1000);
    dbg.print(cnt, "waiting", del, "ms");
    delay(del);
  }

  void handleSlave() {

    // nothing to do @see onFlag
  }

  String getNiceCntStr() const { return String("count : ") + String(cnt, 10); }
  unsigned long sendTime;
  bool isMaster;
};

ProtocolClass protocol;

#undef dbg
