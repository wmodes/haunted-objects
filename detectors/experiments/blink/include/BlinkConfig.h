/**
 * @file BlinkConfig.h
 * @brief Settings that belong only to the blink experiment.
 *
 * Anything generally useful to the piece (such as the serial baud rate) lives in the piece's
 * `Config`; anything only this experiment needs lives here, so deleting the experiment leaves
 * no stray settings behind in main config.
 */
#pragma once

struct BlinkConfig {
  /**
   * GPIO number of the LED that shows the heartbeat. The default is the on-board LED of a
   * typical 30-pin ESP32 dev board (confirmed on the ELEGOO board), so no wiring is needed.
   */
  int statusLedPin = 2;

  /**
   * Time between heartbeat ticks (the LED toggle and the serial line), in milliseconds. Raising
   * it makes the blink slower; lowering it makes it faster and the serial log busier.
   */
  unsigned long heartbeatIntervalMilliseconds = 500;
};

static_assert(BlinkConfig{}.heartbeatIntervalMilliseconds > 0,
              "heartbeatIntervalMilliseconds must be positive");
