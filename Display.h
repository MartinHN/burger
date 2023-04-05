#pragma once

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

  struct DisplayScope {
    DisplayScope(DisplayClass &_o) : o(_o) {
      o.begin();
      // Serial.println("construct Sc");
    }
    DisplayScope(DisplayScope &&from) : o(from.o) {
      //  Serial.println("move Sc");
    }
    ~DisplayScope() { o.end(); }
    DisplayClass &o;
  };

  DisplayScope getScope() { return DisplayScope(*this); }
};

DisplayClass Display;
