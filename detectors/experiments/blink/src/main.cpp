/**
 * @file main.cpp
 * @brief Blink experiment: blinks the status LED and prints a heartbeat over serial.
 *
 * An experiment, not firmware: it exists to prove the toolchain and the layered Config/Secrets
 * work on real hardware. It builds the immutable settings once and hands each part what it needs.
 */
#include <Arduino.h>

#include "BlinkConfig.h"
#include "Config.h"
#include "HeartbeatBlinker.h"
#include "Secrets.h"

namespace {

// Built at compile time. Config and Secrets are the piece's shared settings (shared defaults,
// then piece overrides); BlinkConfig holds only what this experiment needs.
constexpr Config config;
constexpr Secrets secrets;
constexpr BlinkConfig blinkConfig;

HeartbeatBlinker heartbeatBlinker(blinkConfig.statusLedPin,
                                  blinkConfig.heartbeatIntervalMilliseconds, Serial);

}  // namespace

void setup() {
  Serial.begin(config.serialBaudRate);
  Serial.println();
  Serial.println("blink experiment");
  Serial.print("heartbeat interval (ms): ");
  Serial.println(blinkConfig.heartbeatIntervalMilliseconds);

  // Report only whether a credential is set, never its value.
  Serial.print("wifi ssid configured: ");
  Serial.println(secrets.wifiSsid[0] != '\0' ? "yes" : "no");

  heartbeatBlinker.begin();
}

void loop() {
  heartbeatBlinker.update(millis());
}
