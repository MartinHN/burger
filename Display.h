#pragma once
#include "boardDef.h"
#define DISABLE_DISPLAY 0
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
#elif M5STACK_CORE1

struct DisplayClass {
  
  DisplayClass() {

  }

  void setup(){
    M5.Lcd.begin();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lH = M5.Lcd.fontHeight();
  }

  
  void begin() {
    if (!isValid())
      return;
    M5.Lcd.fillScreen(BLACK);
    y = 0;
    // header
    int8_t batt = Power.getBatteryLevel();
    if (batt >= 0) {
      drawLine(String(batt)+"%");
    }
    else 
      drawLine("??");
    
  }

  void end() {
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
