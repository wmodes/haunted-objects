/**
 * @file ChirpRenderer.h
 * @brief Renders a score of timed chirps into mono audio, using each slot's own voice.
 *
 * Each chirp takes its length and pitch from the slot's detector profile, so the voice stays the
 * same however the score was generated. The wave shape and loudness come from the settings.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ConversationConfig.h"
#include "ConversationGenerator.h"
#include "DetectorProfiles.h"

class ChirpRenderer {
 public:
  /**
   * @param audio How to turn chirps into sound (sample rate, loudness, fade, waveform).
   * @param profiles The detector profile table, indexed so that profiles[slot - 1] is that slot.
   * @param profileCount Number of entries in the table.
   */
  ChirpRenderer(const AudioConfig& audio, const DetectorProfile* profiles,
                std::size_t profileCount);

  /**
   * Renders the chirps.
   *
   * @param chirps The score. Chirps starting past the end are ignored.
   * @param durationSeconds Length of the audio to produce.
   * @return Mono 16-bit samples at the configured sample rate.
   */
  std::vector<std::int16_t> render(const std::vector<ChirpEvent>& chirps,
                                   double durationSeconds) const;

 private:
  /** Adds one chirp of `profile`, starting at `startSeconds`, into the float mix. */
  void addChirp(std::vector<float>& mix, const DetectorProfile& profile, double startSeconds) const;

  /** Value of the configured waveform at `phase`, a position within one cycle from 0 to 1. */
  float waveformValue(double phase) const;

  AudioConfig audio_;
  const DetectorProfile* profiles_;
  std::size_t profileCount_;
};
