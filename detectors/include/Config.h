/**
 * @file Config.h
 * @brief The smoke alarm piece's configuration: shared defaults, then this piece's overrides.
 *
 * `Config` starts from every `SharedConfig` default and overwrites the fields this piece needs
 * to change (local overwrites global). It is built once by the entry point and handed to every
 * class that needs a setting; nothing else reads raw values.
 *
 * Only settings that are generally useful to the piece belong here. A setting needed by a single
 * experiment lives in that experiment's own config, so deleting the experiment leaves nothing
 * behind in this file.
 *
 * Validation happens at compile time: a bad value is a build error, not a runtime surprise.
 */
#pragma once

#include "SharedConfig.h"

struct Config : SharedConfig {
  constexpr Config() {
    // No piece-specific overrides yet. Set fields here to overwrite the shared defaults.
  }
};

static_assert(Config{}.serialBaudRate > 0, "serialBaudRate must be positive");
