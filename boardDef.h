#pragma once

// #define PROTO_DUPONT 1 // TTGO
// #define PROTO_DUPONT_DEVKIT 1 // Devkit
#ifndef ARDUINO_M5STACK_Core2
#define M5STACK_CORE1 1
#endif

#define HAS_DISPLAY

#if PROTO_DUPONT
#include "boards.h"
namespace board {
void init() { initBoard(); }
void loop() {}
} // namespace board
#elif PROTO_DUPONT_DEVKIT
namespace board {
void init() {}
void loop() {}
} // namespace board

#elif ARDUINO_M5STACK_Core2
#include <M5Core2.h>
#include "Display.h"
namespace board {
void init() {
  M5.begin(false, false, false, false);
  // M5.Speaker.begin();
  // M5.Speaker.end();
  // Show battery management
  // Power.begin();
  // Power.adaptChargeMode();
  Display.setup();
}
void loop() {
  // Power.adaptChargeMode();
  M5.update();
}
} // namespace board
#elif M5STACK_CORE1

#define SUPPORT_TRANSACTIONS
#include <M5Stack.h>
#ifndef SPI_HAS_TRANSACTION
#error no SPI_HAS_TRANSACTION
#endif
#ifndef SUPPORT_TRANSACTIONS
#error no SUPPORT_TRANSACTIONS
#endif
#ifdef ESP32_PARALLEL
#error has ESP32_PARALLEL
#endif

#if defined(SPI_HAS_TRANSACTION) && defined(SUPPORT_TRANSACTIONS) && !defined(ESP32_PARALLEL)
#pragma ok
#else
#error slow speed display????
#endif
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
void loop() {
  Power.adaptChargeMode();
  M5.update();
}
} // namespace board
#else
#error no board selected
#endif
