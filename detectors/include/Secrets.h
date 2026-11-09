/**
 * @file Secrets.h
 * @brief The smoke alarm piece's credentials: shared values, then this piece's overrides.
 *
 * `Secrets` applies the shared credentials first, then this piece's own, so the local value
 * overwrites the global one. The real values come from two gitignored headers
 * (`secrets_shared.h` and this folder's `secrets_detectors.h`); copy their `.example` templates
 * to create them. It is built once by the entry point and handed to whichever classes need
 * credentials.
 */
#pragma once

#include "SharedSecrets.h"
#include "secrets_shared.h"     // gitignored; defines applySharedSecrets()
#include "secrets_detectors.h"  // gitignored; defines applyDetectorsSecrets()

struct Secrets : SharedSecrets {
  constexpr Secrets() {
    applySharedSecrets(*this);     // global layer first
    applyDetectorsSecrets(*this);  // local layer overwrites it
  }
};
