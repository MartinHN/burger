#pragma once
#ifdef BUTTON_A_PIN
#define BUTTON_PIN BUTTON_A_PIN
#else
#define BUTTON_PIN 0
#endif
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
