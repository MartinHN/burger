#pragma once
#include <RadioLib.h>

auto dbgPhy = Dbg("[phy]");
#define dbg dbgPhy

#define chk(x) chkLoc(x, __LINE__)

IRAM_ATTR void setFlag(void);
const char *getStrErrFromRlib(int c);

struct LoraPhyClass {
  // static constexpr bool implicitHeader = false; // TODO not implicit supported

  static constexpr float loraFreq = 868.0f; // 868.0f;
  static constexpr uint8_t sf = 12;         // defaults to 9 , min 6?
  // defaults to 125 , allowed 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
  static constexpr float bw = 125;
  static constexpr bool hasCrc = true && !(sf == 6);
  static constexpr uint8_t cr = 8;                               // defaults to 7 (coding rate), min 5 , max 8
  static constexpr uint8_t syncWord = RADIOLIB_SX127X_SYNC_WORD; // defaults to RADIOLIB_SX127X_SYNC_WORD
  static constexpr int8_t power = 20;                            // defaults to 10 max 20
  static constexpr uint16_t preambleLength = 8;                  // defaults to 8
  static constexpr uint8_t gain = 1;                             // defaults to 0 , 0 = auto, 1 = max , 6 = min

  volatile bool flagRxOn = false, flagTxOn = false;
  volatile unsigned long flagTxMs = 0, flagRxMs = 0;

  enum class PhyState { None, Tx, Rx };

  PhyState phyState = PhyState::None;

  int16_t lastRlibState = RADIOLIB_ERR_NONE;

  std::function<void()> onRxFlag = {};
  std::function<void()> onTxFlag = {};

  static constexpr bool invertIq = true;
  bool IQBasePolarization = true;

  LoraPhyClass() : radio(new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN)) {}
  void begin() {
    dbg.print("begin");
    Display.drawOneLine("LoRa Slave/Master");

    if (!chk(radio.begin(loraFreq, bw, sf, cr, syncWord, power, preambleLength, gain))) {
      while (1) {
      }
    }

    // if constexpr (implicitHeader)
    //   radio.implicitHeader();
    // else
    //   radio.explicitHeader();
    chk(radio.setCRC(hasCrc));

    radio.setDio0Action(setFlag);
  }

  void end() {
    // TODO?
  }

  bool chkLoc(int16_t state, int ln, const char *successStr = nullptr) noexcept {
    if (state == RADIOLIB_ERR_NONE) {
      if (successStr != nullptr)
        dbg.print(successStr);
    } else {
      dbg.print("failed at", ln, ", code ", getStrErrFromRlib(state));
      Display.drawOneLine(getStrErrFromRlib(state));
      // block a bit on error
      delay(400);
    }
    lastRlibState = state;
    return state == RADIOLIB_ERR_NONE;
  }

  void handle() {
    if (flagTxOn) {
      flagTxOn = false;
      dbg.print("had tx flag", flagTxMs, "ms");
      if (onTxFlag)
        onTxFlag();
    }
    if (flagRxOn) {
      flagRxOn = false;
      dbg.print("had rx flag", flagRxMs, "ms");
      if (onRxFlag)
        onRxFlag();
    }
  }

  void rxMode() {
    dbg.print("setting RX");
    flagRxOn = false;
    flagTxOn = false;
    if (invertIq)
      chk(radio.invertIQ(IQBasePolarization));

    chk(radio.startReceive());
    phyState = PhyState::Rx;
  }

  void txMode() {
    dbg.print("setting TX");
    flagTxOn = false;
    flagRxOn = false;
    if (invertIq)
      chk(radio.invertIQ(!IQBasePolarization));

    // nothing to do?
    phyState = PhyState::Tx;
  }

  uint32_t send(String s) {
    if (phyState != PhyState::Tx)
      txMode();
    lastSentPacketByteLen = s.length();
    lastMessageSent = s;
    chk(radio.startTransmit(s));
    return getTimeOnAirMs(lastSentPacketByteLen);
  }

  uint32_t getTimeOnAirMs(size_t numBytes) const { return radio.getTimeOnAir(numBytes) / 1000; }

  // void sendWaiting(const String &s) {
  //   auto t = send(s);
  //   delay(t / 1000 + 100);
  // }

  void read(String &s) {
    if (phyState != PhyState::Rx)
      dbg.print("was not in Rx mode");
    chk(radio.readData(s));
  }

  SX1276 radio; // eu version
  String lastMessageSent = {};
  size_t lastSentPacketByteLen = 0;
};

LoraPhyClass LoraPhy;

// interrupts
IRAM_ATTR void setFlag(void) {
  // we sent a packet, set the flag
  if (LoraPhy.phyState == LoraPhyClass::PhyState::Tx) {
    LoraPhy.flagTxOn = true;
    LoraPhy.flagTxMs = millis();
  } else if (LoraPhy.phyState == LoraPhyClass::PhyState::Rx) {
    LoraPhy.flagRxOn = true;
    LoraPhy.flagRxMs = millis();
  }
}

const char *getStrErrFromRlib(int c) {
  if (c == RADIOLIB_ERR_NONE)
    return "NONE";
  if (c == RADIOLIB_ERR_UNKNOWN)
    return "UNKNOWN";
  if (c == RADIOLIB_ERR_CHIP_NOT_FOUND)
    return "CHIP_NOT_FOUND";
  if (c == RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED)
    return "MEMORY_ALLOCATION_FAILED";
  if (c == RADIOLIB_ERR_PACKET_TOO_LONG)
    return "PACKET_TOO_LONG";
  if (c == RADIOLIB_ERR_TX_TIMEOUT)
    return "TX_TIMEOUT";
  if (c == RADIOLIB_ERR_RX_TIMEOUT)
    return "RX_TIMEOUT";
  if (c == RADIOLIB_ERR_CRC_MISMATCH)
    return "CRC_MISMATCH";
  if (c == RADIOLIB_ERR_INVALID_BANDWIDTH)
    return "INVALID_BANDWIDTH";
  if (c == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
    return "INVALID_SPREADING_FACTOR";
  if (c == RADIOLIB_ERR_INVALID_CODING_RATE)
    return "INVALID_CODING_RATE";
  if (c == RADIOLIB_ERR_INVALID_BIT_RANGE)
    return "INVALID_BIT_RANGE";
  if (c == RADIOLIB_ERR_INVALID_FREQUENCY)
    return "INVALID_FREQUENCY";
  if (c == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    return "INVALID_OUTPUT_POWER";
  if (c == RADIOLIB_ERR_SPI_WRITE_FAILED)
    return "SPI_WRITE_FAILED";
  if (c == RADIOLIB_ERR_INVALID_CURRENT_LIMIT)
    return "INVALID_CURRENT_LIMIT";
  if (c == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
    return "INVALID_PREAMBLE_LENGTH";
  if (c == RADIOLIB_ERR_INVALID_GAIN)
    return "INVALID_GAIN";
  if (c == RADIOLIB_ERR_WRONG_MODEM)
    return "WRONG_MODEM";
  if (c == RADIOLIB_ERR_INVALID_NUM_SAMPLES)
    return "INVALID_NUM_SAMPLES";
  if (c == RADIOLIB_ERR_INVALID_RSSI_OFFSET)
    return "INVALID_RSSI_OFFSET";
  if (c == RADIOLIB_ERR_INVALID_ENCODING)
    return "INVALID_ENCODING";
  if (c == RADIOLIB_ERR_LORA_HEADER_DAMAGED)
    return "LORA_HEADER_DAMAGED";
  if (c == RADIOLIB_ERR_UNSUPPORTED)
    return "UNSUPPORTED";
  if (c == RADIOLIB_ERR_INVALID_DIO_PIN)
    return "INVALID_DIO_PIN";
  if (c == RADIOLIB_ERR_INVALID_RSSI_THRESHOLD)
    return "INVALID_RSSI_THRESHOLD";
  if (c == RADIOLIB_ERR_NULL_POINTER)
    return "NULL_POINTER";
  if (c == RADIOLIB_ERR_INVALID_BIT_RATE)
    return "INVALID_BIT_RATE";
  if (c == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION)
    return "INVALID_FREQUENCY_DEVIATION";
  if (c == RADIOLIB_ERR_INVALID_BIT_RATE_BW_RATIO)
    return "INVALID_BIT_RATE_BW_RATIO";
  if (c == RADIOLIB_ERR_INVALID_RX_BANDWIDTH)
    return "INVALID_RX_BANDWIDTH";
  if (c == RADIOLIB_ERR_INVALID_SYNC_WORD)
    return "INVALID_SYNC_WORD";
  if (c == RADIOLIB_ERR_INVALID_DATA_SHAPING)
    return "INVALID_DATA_SHAPING";
  if (c == RADIOLIB_ERR_INVALID_MODULATION)
    return "INVALID_MODULATION";
  if (c == RADIOLIB_ERR_INVALID_OOK_RSSI_PEAK_TYPE)
    return "INVALID_OOK_RSSI_PEAK_TYPE";
  if (c == RADIOLIB_ERR_INVALID_SYMBOL)
    return "INVALID_SYMBOL";
  if (c == RADIOLIB_ERR_INVALID_MIC_E_TELEMETRY)
    return "INVALID_MIC_E_TELEMETRY";
  if (c == RADIOLIB_ERR_INVALID_MIC_E_TELEMETRY_LENGTH)
    return "INVALID_MIC_E_TELEMETRY_LENGTH";
  if (c == RADIOLIB_ERR_MIC_E_TELEMETRY_STATUS)
    return "MIC_E_TELEMETRY_STATUS";
  if (c == RADIOLIB_ERR_INVALID_RTTY_SHIFT)
    return "INVALID_RTTY_SHIFT";
  if (c == RADIOLIB_ERR_UNSUPPORTED_ENCODING)
    return "UNSUPPORTED_ENCODING";
  if (c == RADIOLIB_ERR_INVALID_DATA_RATE)
    return "INVALID_DATA_RATE";
  if (c == RADIOLIB_ERR_INVALID_ADDRESS_WIDTH)
    return "INVALID_ADDRESS_WIDTH";
  if (c == RADIOLIB_ERR_INVALID_PIPE_NUMBER)
    return "INVALID_PIPE_NUMBER";
  if (c == RADIOLIB_ERR_ACK_NOT_RECEIVED)
    return "ACK_NOT_RECEIVED";
  if (c == RADIOLIB_ERR_INVALID_NUM_BROAD_ADDRS)
    return "INVALID_NUM_BROAD_ADDRS";
  if (c == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION)
    return "INVALID_CRC_CONFIGURATION";
  if (c == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE)
    return "INVALID_TCXO_VOLTAGE";
  if (c == RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS)
    return "INVALID_MODULATION_PARAMETERS";
  if (c == RADIOLIB_ERR_SPI_CMD_TIMEOUT)
    return "SPI_CMD_TIMEOUT";
  if (c == RADIOLIB_ERR_SPI_CMD_INVALID)
    return "SPI_CMD_INVALID";
  if (c == RADIOLIB_ERR_SPI_CMD_FAILED)
    return "SPI_CMD_FAILED";
  if (c == RADIOLIB_ERR_INVALID_SLEEP_PERIOD)
    return "INVALID_SLEEP_PERIOD";
  if (c == RADIOLIB_ERR_INVALID_RX_PERIOD)
    return "INVALID_RX_PERIOD";
  if (c == RADIOLIB_ERR_INVALID_CALLSIGN)
    return "INVALID_CALLSIGN";
  if (c == RADIOLIB_ERR_INVALID_NUM_REPEATERS)
    return "INVALID_NUM_REPEATERS";
  if (c == RADIOLIB_ERR_INVALID_REPEATER_CALLSIGN)
    return "INVALID_REPEATER_CALLSIGN";
  if (c == RADIOLIB_ERR_RANGING_TIMEOUT)
    return "RANGING_TIMEOUT";
  if (c == RADIOLIB_ERR_INVALID_PAYLOAD)
    return "INVALID_PAYLOAD";
  if (c == RADIOLIB_ERR_ADDRESS_NOT_FOUND)
    return "ADDRESS_NOT_FOUND";

  return "rlib err not found";
}

#undef dbg
