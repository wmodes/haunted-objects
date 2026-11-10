/**
 * @file DetectorProfiles.h
 * @brief The fixed per-slot identity of each detector: how it chirps, every time it powers up.
 *
 * Whatever is plugged into slot 1 always acts like slot 1. Each profile gives a slot its chirp
 * length, chirp frequency, and chirp interval, plus a `model` that records where those numbers
 * came from, so the set of detectors reads as believably varied commercial units.
 *
 * The values were drawn once, at random, within the ranges of the model each slot imitates, and
 * then fixed. Edit them freely; nothing regenerates them.
 *
 * Where the model ranges come from (see the Design Document for links):
 *  - "MC145010-class":       Motorola/NXP MC145010 datasheet; chirp 9.5-11.5 ms, every 38.9-47.1 s.
 *  - "MC146012?-class":      a second Motorola datasheet, part number unconfirmed; chirp
 *                            7.2-8.8 ms, every 28.8-35.2 s.
 *  - "A5348-class":          Allegro A5348 datasheet; chirp 8-12 ms, every 32-48 s.
 *  - "A5366-class":          Allegro/Sensitron A5366 datasheet gives about 10 ms every 43 s; the
 *                            spread around those two figures (9-11 ms, 41-45 s) is assumed.
 *  - "Kidde-style":          interval 30-40 s from Kidde support pages; chirp length assumed
 *                            (8-12 ms) from the chip datasheets above.
 *  - "BRK/First-Alert-style": interval about once a minute (55-60 s) from BRK and First Alert
 *                            pages, capped at 60 s by NFPA 72; chirp length assumed (8-12 ms).
 *
 * Chirp frequency comes from patent test results and a product manual, not from a detector
 * datasheet: typical smoke-alarm horns sit near 3100-3200 Hz (about 3200 Hz +/-10%; one manual
 * states 3.2 +/-0.3 kHz; a patent puts typical horn output at 3000-4000 Hz). Frequencies were
 * drawn at random from 3000-3800 Hz, trimming the low tail that sounded unlikely. Slots 1-6, the
 * voices a conversation casts from for now, are at least 120 Hz apart so each is clearly
 * distinguishable (slots 1 and 2 were kept; slots 3-6 were drawn to fit). Slots 7-12 are not yet
 * tuned: no two slots anywhere are closer than 30 Hz, but several pairs involving slots 7-12 are
 * exactly that close (for example slots 1 and 7), so do not cast from them without checking. Each
 * physical horn has its own resonant sweet spot and will likely bunch near 3-3.5 kHz, so treat
 * these as starting points to tune by ear once the horns are wired.
 */
#pragma once

#include <cstddef>

/** One slot's fixed chirp identity. */
struct DetectorProfile {
  /** Position of the detector, counted from 1. Slot 1 always acts like slot 1. */
  unsigned slot;

  /** Which kind of commercial detector this slot imitates, and so where its values came from. */
  const char* model;

  /** Length of one chirp, in milliseconds. Raising it makes each chirp longer. */
  float chirpLengthMilliseconds;

  /** Pitch of the chirp, in hertz. Raising it makes the chirp higher. */
  unsigned chirpFrequencyHertz;

  /**
   * Time between the starts of successive chirps in the ambient low-battery suite, in seconds.
   * Raising it makes chirps rarer. This is timing, not voice: a slot's voice is its chirp length
   * and chirp frequency, which every suite shares, while each other suite brings its own timing.
   */
  float chirpIntervalSeconds;
};

/**
 * The profile of every slot, in slot order. Twelve slots leaves headroom: the real count depends
 * on how many alarms are built (push-pull drive spends two pins per horn).
 */
inline constexpr DetectorProfile kDetectorProfiles[] = {
    {1, "A5348-class", 12.0f, 3140, 35.3f},
    {2, "A5366-class", 11.0f, 3670, 41.6f},
    {3, "Kidde-style", 11.4f, 3440, 33.0f},
    {4, "BRK/First-Alert-style", 9.5f, 3020, 57.0f},
    {5, "MC145010-class", 10.3f, 3320, 44.6f},
    {6, "Kidde-style", 8.8f, 3790, 37.0f},
    {7, "MC146012?-class", 8.6f, 3110, 29.7f},
    {8, "BRK/First-Alert-style", 10.5f, 3640, 58.8f},
    {9, "A5348-class", 10.2f, 3290, 35.0f},
    {10, "A5366-class", 9.2f, 3050, 41.6f},
    {11, "MC145010-class", 10.9f, 3350, 39.4f},
    {12, "MC146012?-class", 8.5f, 3760, 34.4f},
};

/** Number of slots in the table. */
inline constexpr std::size_t kDetectorProfileCount =
    sizeof(kDetectorProfiles) / sizeof(kDetectorProfiles[0]);

namespace detector_profiles_internal {

/**
 * Checks the whole table at compile time: slots count up from 1 with no gaps, and every chirp
 * has a positive length, a positive frequency, and an interval longer than the chirp itself.
 * A bad entry is a build error, not a runtime surprise.
 */
constexpr bool tableIsValid() {
  for (std::size_t i = 0; i < kDetectorProfileCount; ++i) {
    const DetectorProfile& profile = kDetectorProfiles[i];
    const bool slotIsInOrder = profile.slot == i + 1;
    const bool lengthIsPositive = profile.chirpLengthMilliseconds > 0.0f;
    const bool frequencyIsPositive = profile.chirpFrequencyHertz > 0;
    // The interval is in seconds and the length in milliseconds, hence the factor of 1000.
    const bool intervalExceedsLength =
        profile.chirpIntervalSeconds * 1000.0f > profile.chirpLengthMilliseconds;
    if (!(slotIsInOrder && lengthIsPositive && frequencyIsPositive && intervalExceedsLength)) {
      return false;
    }
  }
  return true;
}

}  // namespace detector_profiles_internal

static_assert(detector_profiles_internal::tableIsValid(),
              "kDetectorProfiles: slots must count up from 1, and each chirp must have positive "
              "length and frequency and an interval longer than the chirp");
