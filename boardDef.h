#pragma once

// #define PROTO_DUPONT 1 // TTGO
#define M5STACK_CORE1 1

#define HAS_DISPLAY

#if PROTO_DUPONT
#include "boards.h"
namespace board {
void init() { initBoard(); }
void loop() {}
} // namespace board
#elif M5STACK_CORE1
#include <M5Stack.h>
#include "boardUtils/M5/Power.h"
#include "boardUtils/M5/Power.cpp"
#include "Display.h"
namespace board {
void init() {
  M5.begin(false, false, false, false);
  M5.Speaker.begin();
  M5.Speaker.end();
  // Show battery management
  Power.begin();
  Power.adaptChargeMode();
  Display.setup();
}
void loop() { Power.adaptChargeMode(); }
} // namespace board
#else
#error no board selected
#endif
