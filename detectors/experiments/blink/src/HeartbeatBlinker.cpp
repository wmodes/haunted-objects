/**
 * @file HeartbeatBlinker.cpp
 * @brief Implementation of HeartbeatBlinker. See HeartbeatBlinker.h.
 */
#include "HeartbeatBlinker.h"

HeartbeatBlinker::HeartbeatBlinker(int ledPin, unsigned long intervalMilliseconds, Print& log)
    : ledPin_(ledPin), intervalMilliseconds_(intervalMilliseconds), log_(log) {}

void HeartbeatBlinker::begin() {
  pinMode(ledPin_, OUTPUT);
  digitalWrite(ledPin_, LOW);
}

void HeartbeatBlinker::update(unsigned long nowMilliseconds) {
  // Unsigned subtraction stays correct when millis() wraps around after ~49 days.
  if (nowMilliseconds - lastBeatMilliseconds_ < intervalMilliseconds_) {
    return;
  }

  lastBeatMilliseconds_ = nowMilliseconds;
  ledIsOn_ = !ledIsOn_;
  ++beatCount_;
  digitalWrite(ledPin_, ledIsOn_ ? HIGH : LOW);

  log_.print("heartbeat #");
  log_.print(beatCount_);
  log_.print(" led ");
  log_.println(ledIsOn_ ? "on" : "off");
}
