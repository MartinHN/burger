#pragma once
#include "boardDef.h"
#if PROTO_DUPONT_DEVKIT
#define DISABLE_DISPLAY 1
#else
#define DISABLE_DISPLAY 0
#endif
#if DISABLE_DISPLAY

struct DisplayClass {
  
  DisplayClass() {

  }

  void setup(){}

  void begin() {}

  void end() {}
  void drawOneLine(const char *buf, bool serial = true) {}

  void drawLine(const String &s) {}
  void drawLine(const char *buf) {}


  bool isValid() const {
    return true;
  }
  
};
#elif M5STACK_CORE1 || ARDUINO_M5STACK_Core2

struct DisplayClass {

  DisplayClass() {}
  String headerInfos;
  void setup() {
    M5.Lcd.begin();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lH = M5.Lcd.fontHeight();
    setup_t tft_settings;
    M5.Lcd.getSetup(tft_settings);
    screen_width = tft_settings.tft_height;
    screen_height = tft_settings.tft_width;
  }

  void drawDick() {
    M5.Lcd.fillScreen(BLACK);
    int32_t bR = screen_width / 6;
    int32_t midX = screen_width / 2;
    int32_t dw = screen_width / 4;
    uint32_t col = TFT_WHITE;
    M5.Lcd.fillRoundRect(midX - dw / 2, 0, dw, screen_height - bR, dw / 2, col);
    M5.Lcd.fillCircle(midX - bR, screen_height - bR, bR, col);
    M5.Lcd.fillCircle(midX + bR, screen_height - bR, bR, col);
  }

  void drawHeader() {

// header
#if ARDUINO_M5STACK_Core2
    int8_t batt = M5.Axp.GetBatteryLevel();
#else
    int8_t batt = Power.getBatteryLevel();
#endif
    if (batt >= 0) {
      drawLine(String(batt) + "% " + headerInfos);
    } else
      drawLine("??");
  }

  void begin() {
    if (!isValid())
      return;

    M5.Lcd.TFT_eSPI::startWrite();
    M5.Lcd.fillScreen(BLACK);

    y = 0;
    drawHeader();
  }

  void end() {
    M5.Lcd.TFT_eSPI::endWrite();
    if (!isValid())
      return;
    // nothing?
  }
  void drawOneLine(const char *buf, bool serial = true) {
    if (serial)
      Serial.println(buf);
    begin();
    drawLine(buf);
    end();
  }

  void drawLine(const String &s) { drawLine(s.c_str()); }
  void drawLine(const char *buf) {
    y += lH;
    if (!isValid())
      return;

    M5.Lcd.setCursor(5, y);
    M5.Lcd.println(buf);
  }


  bool isValid() const {
    // if (!u8g2) {
    //   Serial.println("Display not available");
    //   return false;
    // }
    return true;
  }
  unsigned short lH = 12;
  unsigned short y = 0;
  int32_t screen_height = 0;
  int32_t screen_width = 0;
};

#else
struct DisplayClass {
  DisplayClass() {}
  void begin() {
    if (!isValid())
      return;
    u8g2->clearBuffer();
    y = 0;
  }

  void drawOneLine(const char *buf, bool serial = true) {
    if (serial)
      Serial.println(buf);
    begin();
    drawLine(buf);
    end();
  }

  void drawLine(const String &s) { drawLine(s.c_str()); }
  void drawLine(const char *buf) {
    y += lH;
    if (!isValid())
      return;
    u8g2->drawStr(0, y, buf);
  }

  void end() {
    if (!isValid())
      return;
    u8g2->sendBuffer();
  }

  bool isValid() const {
    if (!u8g2) {
      Serial.println("Display not available");
      return false;
    }
    return true;
  }
  unsigned short lH = 12;
  unsigned short y = 0;
};

#endif

DisplayClass Display;
struct DisplayScope {
  static DisplayScope get() { return DisplayScope(Display); }
  DisplayScope(DisplayClass &_o) : o(_o) {
    o.begin();
    // Serial.println("construct Sc");
  }
  ~DisplayScope() { o.end(); }
  DisplayClass &o;
};
