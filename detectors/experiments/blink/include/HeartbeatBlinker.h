/**
 * @file HeartbeatBlinker.h
 * @brief Bring-up helper: blinks an LED and logs a line on every beat.
 *
 * Proves the whole chain works on real hardware (build, flash, config, serial) before any horn
 * is wired. It never blocks: `update()` is called from the main loop and acts only when the
 * configured interval has elapsed.
 */
#pragma once

#include <Arduino.h>

class HeartbeatBlinker {
 public:
  /**
   * @param ledPin Pin of the LED to toggle on each beat.
   * @param intervalMilliseconds Time between beats.
   * @param log Where each beat is reported (e.g. the serial port); injected so this class never
   *            depends on a specific output device.
   */
  HeartbeatBlinker(int ledPin, unsigned long intervalMilliseconds, Print& log);

  /** Prepares the LED pin. Call once from setup(). */
  void begin();

  /**
   * Toggles the LED and logs a line if the interval has elapsed since the last beat.
   * Call on every pass of the main loop.
   *
   * @param nowMilliseconds Current time, from millis(); passed in so the caller owns the clock.
   */
  void update(unsigned long nowMilliseconds);

 private:
  int ledPin_;
  unsigned long intervalMilliseconds_;
  Print& log_;
  unsigned long lastBeatMilliseconds_ = 0;
  unsigned long beatCount_ = 0;
  bool ledIsOn_ = false;
};
