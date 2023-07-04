#pragma once
#define BUTTON_PIN 0

struct BtnWatcher {
  short pin;
  bool isInverted;
  BtnWatcher(short _pin = BUTTON_PIN, bool _isInverted = true) : pin(_pin), isInverted(_isInverted) { pinMode(pin, INPUT); }
  std::function<void(bool)> onButton = {};
  void handle() {
    bool state = digitalRead(pin);
    if (isInverted)
      state = !state;

    if (lastState != state) {
      lastState = state;
      if (onButton)
        onButton(state);
    }
  }

  bool lastState = false;
};
