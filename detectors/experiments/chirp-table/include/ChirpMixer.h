/**
 * @file ChirpMixer.h
 * @brief Renders the chirps of several detector slots into one mono audio mix.
 *
 * Each slot chirps at its own profile's length, pitch, and interval. Every slot starts after its
 * own random warmup offset, drawn from 0 up to that slot's chirp interval, so two slots of the
 * same model (or even with identical intervals) do not chirp in unison.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ChirpTableConfig.h"
#include "DetectorProfiles.h"

/** One chirp that was placed in the mix. */
struct ChirpEvent {
  unsigned slot;
  double startSeconds;
};

class ChirpMixer {
 public:
  /**
   * @param config Experiment settings (slot range, duration, waveform, seed, ...).
   * @param profiles The detector profile table, indexed so that profiles[slot - 1] is that slot.
   * @param profileCount Number of entries in the table.
   */
  ChirpMixer(const ChirpTableConfig& config, const DetectorProfile* profiles,
             std::size_t profileCount);

  /**
   * Renders the mix.
   *
   * @return Mono 16-bit samples at the configured sample rate, `durationSeconds` long.
   */
  std::vector<std::int16_t> render();

  /** Every chirp placed by the last render(), in the order placed (slot by slot). */
  const std::vector<ChirpEvent>& events() const { return events_; }

  /** The warmup offset drawn for each slot by the last render(), in slot order. */
  const std::vector<double>& warmupOffsetsSeconds() const { return warmupOffsetsSeconds_; }

 private:
  /** Adds one chirp of `profile`, starting at `startSeconds`, into the float mix. */
  void addChirp(std::vector<float>& mix, const DetectorProfile& profile, double startSeconds) const;

  /** Value of the configured waveform at `phase`, a position within one cycle from 0 to 1. */
  float waveformValue(double phase) const;

  ChirpTableConfig config_;
  const DetectorProfile* profiles_;
  std::size_t profileCount_;
  std::vector<ChirpEvent> events_;
  std::vector<double> warmupOffsetsSeconds_;
};
