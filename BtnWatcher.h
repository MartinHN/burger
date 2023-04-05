#pragma once
#define BUTTON_PIN 0

struct BtnWatcher {
  short pin;
  BtnWatcher(short _pin = BUTTON_PIN) : pin(_pin) { pinMode(pin, INPUT); }
  std::function<void(bool)> onButton = {};
  void handle() {
    bool state = digitalRead(pin);
    if (lastState != state) {
      lastState = state;
      if (onButton)
        onButton(state);
    }
  }

  bool lastState = false;
};
