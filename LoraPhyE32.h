#define LoRa_E32_DEBUG 1
#include <LoRa_E32.h>

auto dbgPhy = Dbg("[phy]");
#define dbg dbgPhy

byte lastPhyRespStatus = 1;
bool chkLoc(byte status, int ln, const char *successStr = nullptr) noexcept {
  if (status == 1) {
    if (successStr != nullptr)
      dbg.print(successStr);
  } else {
    dbg.print("failed at", ln, ", code ", getResponseDescriptionByParams(status));
    auto lk = Display.getScope();
    Display.drawOneLine(getResponseDescriptionByParams(status).c_str());
    // block a bit on error
    delay(400);
  }
  lastPhyRespStatus = status;
  return status == 1;
}

#define chk(x) chkLoc((x).code, __LINE__)
// ---------- esp32 pins --------------

struct LoraPhyClass {
  // static constexpr bool implicitHeader = false; // TODO not implicit supported

  static constexpr float loraFreq = 868.0f; // 868.0f;
  static constexpr uint8_t sf = 12;         // defaults to 9 , min 6?
  // defaults to 125 , allowed 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
  static constexpr float bw = 125;
  //   static constexpr bool hasCrc = true && !(sf == 6);
  //   static constexpr uint8_t cr = 8;                               // defaults to 7 (coding rate), min 5 , max 8
  //   static constexpr uint8_t syncWord = RADIOLIB_SX127X_SYNC_WORD; // defaults to RADIOLIB_SX127X_SYNC_WORD
  //   static constexpr int8_t power = 20;                            // defaults to 10 max 20
  //   static constexpr uint16_t preambleLength = 8;                  // defaults to 8
  //   static constexpr uint8_t gain = 1;                             // defaults to 0 , 0 = auto, 1 = max , 6 = min

  volatile unsigned long flagTxMs = 0, flagRxMs = 0;
  std::function<void()> onRxFlag = {};
  std::function<void()> onTxFlag = {};
  enum class PhyState { None, Tx, Rx };

  PhyState phyState = PhyState::None;

  byte espTxPin = 12;
  byte espRxPin = 25;
  byte auxPin = 36;
  LoRa_E32 e32ttl;

  // LoRa_E32( txE32pin,  rxE32pin, HardwareSerial*,  auxPin,  m0Pin,  m1Pin,  bpsRate,  serialConfig = SERIAL_8N1);
  LoraPhyClass() : e32ttl(espRxPin, espTxPin, &Serial2, auxPin, 21, 13, UART_BPS_RATE_9600) {}
  void begin() {
    dbg.print("begin");
    Display.drawOneLine("LoRa Slave/Master");

    if (!e32ttl.begin()) {
      dbg.print("can't start EByte");
      while (1) {
      }
    }
    pinMode(espRxPin, INPUT_PULLUP);
    pinMode(auxPin, INPUT_PULLUP);

    delay(400);

    while (!printConfig()) {
      dbg.print("no config found");
      {
        auto lk = Display.getScope();
        Display.drawOneLine("no com with E32");
      }
      delay(1000);
    }
    /*
        ResponseStructContainer c;
        c = e32ttl.getConfiguration();
        // It's important get configuration pointer before all other operation
        Configuration configuration = *(Configuration *)c.data;
        Serial.println(c.status.getResponseDescription());
        Serial.println(c.status.code);

        configuration.ADDL = 0x0;
        configuration.ADDH = 0x1;
        configuration.CHAN = 0x19;

        configuration.OPTION.fec = FEC_0_OFF;
        configuration.OPTION.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
        configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS;
        configuration.OPTION.transmissionPower = POWER_17;
        configuration.OPTION.wirelessWakeupTime = WAKE_UP_1250;

        configuration.SPED.airDataRate = AIR_DATA_RATE_011_48;
        configuration.SPED.uartBaudRate = UART_BPS_115200;
        configuration.SPED.uartParity = MODE_00_8N1;

        // Set configuration changed and set to not hold the configuration
        ResponseStatus rs = e32ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_LOSE);
        Serial.println(rs.getResponseDescription());
        Serial.println(rs.code);
        */
  }

  void end() {
    // TODO?
  }

  void handle() {
    if (phyState != PhyState::Rx)
      return;
    if (e32ttl.available() > 0) {
      flagRxMs = millis();
      if (onRxFlag)
        onRxFlag();
    }
  }

  uint32_t getTimeOnAirMs(size_t numBytes) const {
    return 1000; // TODO
  }

  void rxMode() {
    // dbg.print("setting RX");
    phyState = PhyState::Rx;
  }

  void txMode() {
    // dbg.print("setting TX");
    // nothing to do?
    phyState = PhyState::Tx;
  }

  uint32_t send(String s) {
    lastSentPacketByteLen = s.length();
    lastMessageSent = s;
    // Send message
    chk(e32ttl.sendMessage(s));
    flagTxMs = millis();
    if (onTxFlag)
      onTxFlag();
    return 0; // TODO: radio.getTimeOnAir(lastSentPacketByteLen);
  }

  void read(String &s) {

    ResponseContainer rs = e32ttl.receiveMessage();
    if (!chk(rs.status))
      return;

    s = rs.data;
  }

  bool printConfig() {
    ResponseStructContainer c;
    c = e32ttl.getConfiguration();
    if (!chk(c.status)) {
      return false;
    }
    // It's important get configuration pointer before all other operation
    Configuration configuration = *(Configuration *)c.data;
    dbg.printWithSpace(F("Chan : "));
    Serial.print(configuration.CHAN, DEC);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.getChannelDescription());
    dbg.print(F(" "));
    dbg.printWithSpace(F("SpeedParityBit     : "));
    Serial.print(configuration.SPED.uartParity, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getUARTParityDescription());
    dbg.printWithSpace(F("SpeedUARTDatte  : "));
    Serial.print(configuration.SPED.uartBaudRate, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getUARTBaudRate());
    dbg.printWithSpace(F("SpeedAirDataRate   : "));
    Serial.print(configuration.SPED.airDataRate, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getAirDataRate());

    dbg.printWithSpace(F("OptionTrans        : "));
    Serial.print(configuration.OPTION.fixedTransmission, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getFixedTransmissionDescription());
    dbg.printWithSpace(F("OptionPullup       : "));
    Serial.print(configuration.OPTION.ioDriveMode, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getIODroveModeDescription());
    dbg.printWithSpace(F("OptionWakeup       : "));
    Serial.print(configuration.OPTION.wirelessWakeupTime, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getWirelessWakeUPTimeDescription());
    dbg.printWithSpace(F("OptionFEC          : "));
    Serial.print(configuration.OPTION.fec, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getFECDescription());
    dbg.printWithSpace(F("OptionPower        : "));
    Serial.print(configuration.OPTION.transmissionPower, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getTransmissionPowerDescription());
    return true;
  }

  String lastMessageSent = {};
  size_t lastSentPacketByteLen = 0;
};

LoraPhyClass LoraPhy;
