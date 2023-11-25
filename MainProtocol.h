#include "Dbg.h"
#include "LoraPhyE32.h"
// #include "../../momo/relaystrio/LoraPhyE32.h"
#include "lib/cobs.h"
auto dbgCntr = Dbg("[lora]");
#define dbg dbgCntr

bool shouldDisplayLoraMsg = true;

void displayMsg(const String &m) {
  if (!shouldDisplayLoraMsg)
    return;
  auto lk = DisplayScope::get();
  Display.drawLine(m);
  dbg.print(m);
}

struct LoraApp {

  void flushSerial() {
    while (HWSerial.available())
      HWSerial.read();
    HWSerial.flush();
  }
  typedef enum { SYNC = 1, PING, PONG, ACTIVATE, DISABLE_AGENDA, FILE_MSG } MESSAGE_TYPE;

  void begin() {

    LoraPhy.begin();

    LoraPhy.rxMode();
    // LoraPhy.onTxFlag = []() {
    //   // keep listening
    //   LoraPhy.rxMode();
    // };

    LoraPhy.onRxFlag = [this] {
      // you can read received data as an Arduino String

      uint8_t type = 99;
      if (!LoraPhy.readNext(type)) {
        displayMsg("corrupted lora msg no type to read");
        flushSerial();
        return;
      }
      dbg.print("new msg", type);

      if ((type == MESSAGE_TYPE::PING)) {
        String disp;
        uint8_t c;
        while (LoraPhy.readNext(c)) {
          disp += " " + String(c);
        }
        displayMsg("ping" + disp);
      } else if (type == MESSAGE_TYPE::SYNC) {
        displayMsg("sync " + LoraPhy.readString());
      } else if (type == MESSAGE_TYPE::PONG) {
        uint8_t uuid = 0;
        uint8_t act = 0;
        if (!LoraPhy.readNext(uuid) || !LoraPhy.readNext(act)) {
          displayMsg("corrupted lora msg not type to read");
          return;
        }
        auto md5 = LoraPhy.readString();
        String disp = "pong " + String(uuid) + " " + String(act) + " " + md5;
        uint8_t mis = 0;
        while (LoraPhy.readNext(mis)) {
          disp += " ";
          disp += String(mis);
        }
        displayMsg(disp);
      } else if (type == MESSAGE_TYPE::ACTIVATE) {
        String disp;
        uint8_t c;
        while (LoraPhy.readNext(c))
          disp += " " + String(c);
        displayMsg("activate " + disp);
      } else if (type == MESSAGE_TYPE::DISABLE_AGENDA) {
        String disp;
        uint8_t c;
        while (LoraPhy.readNext(c))
          disp += " " + String(c);
        displayMsg("disable agenda " + disp);
      } else if (type == MESSAGE_TYPE::FILE_MSG) {
        String disp;
        uint8_t c;
        LoraPhy.readNext(c);
        if (c == 255)
          disp += "Start";
        else {
          disp += "num : ";
          disp += String(int(c), 10);
        }
        displayMsg("File " + disp);
      } else {
        displayMsg("message not supported");
      }
      flushSerial();
      dbg.print("end lora msg");
    };
  }

  void end() { LoraPhy.end(); }

  void handle() { LoraPhy.handle(); }
}; // namespace LoraApp

LoraApp protocol;
#undef dbg
