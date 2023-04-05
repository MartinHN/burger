#pragma once
struct Dbg {
  Dbg(const char *_prefix) : prefix(_prefix) {}

  template <typename... Args> void print(Args... a) {
    Serial.print(prefix);
    Serial.print(" : ");
    (printWithSpace(a), ...);
    Serial.println("");
  }
  template <typename Arg> void printWithSpace(Arg a) {
    Serial.print(a);
    Serial.print(" ");
  }
  const char *prefix;
};
