#include "boards.h"

/// local
#include "Dbg.h"
#include "Conf.h"
#include "Display.h"
#include "LoraPhy.h"
#include "tests/SimpleCounter.h"

auto dbg = Dbg("[main]");

int counter = 0;

void setup() {
  initBoard();
  // When the power is turned on, a delay is required.
  delay(1500);
  // LoraPhy.begin();
  dbg.print("start");

  dbg.print("has", String("started"), 1);
  protocol.begin();
}

void loop() {
  // LoraPhy.handle();
  protocol.handle();
  delay(100);
}

/*
void sendCountPkt() {
  Serial.print("Sending packet: ");
  Serial.println(counter);
  unsigned long long tbeforeSend = millis();
  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();
  auto deltaSend = millis() - tbeforeSend;

  Display.begin();
  char buf[256];
  snprintf(buf, sizeof(buf), "SF: %d ,bw: %d", LoRa.getSpreadingFactor(), LoRa.getSignalBandwidth());
  Display.drawLine(buf);
  Display.drawLine("Transmitting: OK!");
  snprintf(buf, sizeof(buf), "Sending: %d (%d)", counter, deltaSend);
  Display.drawLine(buf);
  Display.end();
  counter++;
}
*/
