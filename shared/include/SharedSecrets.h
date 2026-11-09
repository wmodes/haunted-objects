/**
 * @file SharedSecrets.h
 * @brief The shared layer of secrets: the credential fields every haunted object may need.
 *
 * This file holds only the shape and empty defaults, so it is safe to commit. The real values
 * live in the gitignored `secrets_shared.h` (copy `secrets_shared.h.example` to create it).
 * A piece's `Secrets` applies the shared values first and its own values after, so the local
 * value overwrites the global one.
 */
#pragma once

struct SharedSecrets {
  /** Name of the WiFi network to join. Empty means not set. */
  const char* wifiSsid = "";

  /** Password of the WiFi network. Empty means not set. */
  const char* wifiPassword = "";

  /** Password protecting over-the-air firmware updates. Empty means not set. */
  const char* otaPassword = "";
};
