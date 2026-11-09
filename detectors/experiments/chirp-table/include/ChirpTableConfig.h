/**
 * @file ChirpTableConfig.h
 * @brief Settings that belong only to the chirp-table experiment.
 *
 * Anything generally useful to the piece lives in the piece's own config; anything only this
 * experiment needs lives here, so deleting the experiment leaves no stray settings behind.
 */
#pragma once

#include <cstddef>

#include "DetectorProfiles.h"

/** The shape of the wave rendered for each chirp. */
enum class Waveform {
  Sine,      ///< Smooth tone; closest to what a horn's resonant cavity radiates.
  Square,    ///< What a push-pull H-bridge drives onto the horn electrically.
  Sawtooth,  ///< A rising ramp; buzzier, with both odd and even harmonics.
};

struct ChirpTableConfig {
  /** First slot to render. Slots count from 1, as in the profile table. */
  unsigned firstSlot = 1;

  /** Last slot to render. Slots firstSlot through lastSlot chirp together. */
  unsigned lastSlot = 6;

  /**
   * Length of the rendered audio, in seconds. Chirp intervals are 29-58 s, so a longer run lets
   * each slot chirp more times and shows how the slots drift in and out of step.
   */
  double durationSeconds = 150.0;

  /** Audio sample rate, in hertz. Raising it improves quality and grows the file. */
  unsigned sampleRateHertz = 44100;

  /**
   * Overall loudness of the mix, from 0 (silent) to 1 (full scale). Raising it makes the chirps
   * louder; overlapping chirps are clipped at 1.
   */
  double amplitude = 0.5;

  /**
   * Length of the fade-in and fade-out on each chirp, in milliseconds. Without a fade the abrupt
   * start and end of a short burst clicks. Raising it softens the chirp; it is capped at half the
   * chirp's length.
   */
  double fadeMilliseconds = 1.0;

  /**
   * Which wave each chirp is drawn with. Sine is the default here because this experiment plays
   * through computer speakers. The detector itself is expected to drive its horns with Square
   * (what the H-bridge produces natively); set Square to hear that wave on the speakers.
   */
  Waveform waveform = Waveform::Sine;

  /**
   * Seed for the random warmup offsets. The same seed gives the same offsets, so a run can be
   * reproduced; change it to hear a different arrangement.
   */
  unsigned randomSeed = 20261110;

  /** Where the WAV file is written, relative to where the program is run. */
  const char* outputFilePath = "chirp-table.wav";
};

static_assert(ChirpTableConfig{}.firstSlot >= 1, "firstSlot counts from 1");
static_assert(ChirpTableConfig{}.firstSlot <= ChirpTableConfig{}.lastSlot,
              "firstSlot must not come after lastSlot");
static_assert(ChirpTableConfig{}.lastSlot <= kDetectorProfileCount,
              "lastSlot must be a slot in the profile table");
static_assert(ChirpTableConfig{}.durationSeconds > 0.0, "durationSeconds must be positive");
static_assert(ChirpTableConfig{}.sampleRateHertz > 0, "sampleRateHertz must be positive");
static_assert(ChirpTableConfig{}.amplitude > 0.0 && ChirpTableConfig{}.amplitude <= 1.0,
              "amplitude must be in (0, 1]");
static_assert(ChirpTableConfig{}.fadeMilliseconds >= 0.0, "fadeMilliseconds must not be negative");
