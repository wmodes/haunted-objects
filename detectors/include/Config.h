/**
 * @file Config.h
 * @brief The smoke alarm piece's configuration: shared defaults, then this piece's overrides.
 *
 * `Config` starts from every `SharedConfig` default and overwrites the fields this piece needs
 * to change (local overwrites global). It is built once by the entry point and handed to every
 * class that needs a setting; nothing else reads raw values.
 *
 * Validation happens at compile time: a bad value is a build error, not a runtime surprise.
 */
#pragma once

#include "SharedConfig.h"

struct Config : SharedConfig {
  constexpr Config() {
    // Faster than the shared default so the heartbeat is easy to see during bring-up.
    heartbeatIntervalMilliseconds = 500;
  }
};

static_assert(Config{}.serialBaudRate > 0, "serialBaudRate must be positive");
static_assert(Config{}.heartbeatIntervalMilliseconds > 0,
              "heartbeatIntervalMilliseconds must be positive");
