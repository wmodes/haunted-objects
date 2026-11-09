/**
 * @file SharedConfig.h
 * @brief The shared layer of configuration: defaults every haunted object starts from.
 *
 * Each piece builds its own `Config` on top of this struct and overrides only the fields it needs
 * to change, so the local value overwrites the global one. This file holds defaults only; it
 * never reads anything and never knows which piece it is used by.
 *
 * Every key's name carries its unit. Fields are plain values resolved at compile time.
 */
#pragma once

struct SharedConfig {
  /**
   * Speed of the serial link to the computer, in baud. The serial monitor must be set to the
   * same value or its output is garbled.
   */
  unsigned long serialBaudRate = 115200;

  /**
   * Time between heartbeat ticks (the status LED toggle and the serial heartbeat line), in
   * milliseconds. Raising it makes the heartbeat slower; lowering it makes it faster and the
   * serial log busier.
   */
  unsigned long heartbeatIntervalMilliseconds = 1000;
};
