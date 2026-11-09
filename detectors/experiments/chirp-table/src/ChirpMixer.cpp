/**
 * @file ChirpMixer.cpp
 * @brief Implementation of ChirpMixer. See ChirpMixer.h.
 */
#include "ChirpMixer.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr double kPi = 3.14159265358979323846;

/** Full-scale value of a signed 16-bit sample. */
constexpr double kInt16FullScale = 32767.0;

}  // namespace

ChirpMixer::ChirpMixer(const ChirpTableConfig& config, const DetectorProfile* profiles,
                       std::size_t profileCount)
    : config_(config), profiles_(profiles), profileCount_(profileCount) {}

float ChirpMixer::waveformValue(double phase) const {
  switch (config_.waveform) {
    case Waveform::Sine:
      return static_cast<float>(std::sin(2.0 * kPi * phase));
    case Waveform::Square:
      return phase < 0.5 ? 1.0f : -1.0f;
    case Waveform::Sawtooth:
      return static_cast<float>(2.0 * phase - 1.0);
  }
  return 0.0f;
}

void ChirpMixer::addChirp(std::vector<float>& mix, const DetectorProfile& profile,
                          double startSeconds) const {
  const double rate = config_.sampleRateHertz;
  const std::size_t startSample = static_cast<std::size_t>(std::llround(startSeconds * rate));
  const std::size_t lengthSamples =
      static_cast<std::size_t>(std::llround(profile.chirpLengthMilliseconds / 1000.0 * rate));

  // The fade is capped at half the chirp so the fade-in and fade-out never overlap.
  const std::size_t fadeSamples = std::min(
      static_cast<std::size_t>(std::llround(config_.fadeMilliseconds / 1000.0 * rate)),
      lengthSamples / 2);

  for (std::size_t i = 0; i < lengthSamples && startSample + i < mix.size(); ++i) {
    // Phase restarts at zero for every chirp: each is a fresh burst, like a horn being enabled.
    const double cycles = static_cast<double>(i) * profile.chirpFrequencyHertz / rate;
    const double phase = cycles - std::floor(cycles);

    double envelope = 1.0;
    if (fadeSamples > 0) {
      const double fadeIn = static_cast<double>(i) / fadeSamples;
      const double fadeOut = static_cast<double>(lengthSamples - 1 - i) / fadeSamples;
      envelope = std::min({1.0, fadeIn, fadeOut});
    }

    mix[startSample + i] += static_cast<float>(waveformValue(phase) * envelope);
  }
}

std::vector<std::int16_t> ChirpMixer::render() {
  events_.clear();
  warmupOffsetsSeconds_.clear();

  const std::size_t totalSamples =
      static_cast<std::size_t>(std::llround(config_.durationSeconds * config_.sampleRateHertz));
  std::vector<float> mix(totalSamples, 0.0f);

  // One generator for the whole run, consumed in slot order, so a seed fixes every offset.
  std::mt19937 rng(config_.randomSeed);

  for (unsigned slot = config_.firstSlot; slot <= config_.lastSlot && slot <= profileCount_;
       ++slot) {
    const DetectorProfile& profile = profiles_[slot - 1];

    // Warmup offset: the slot starts somewhere within its own first interval.
    std::uniform_real_distribution<double> warmup(0.0, profile.chirpIntervalSeconds);
    const double warmupOffset = warmup(rng);
    warmupOffsetsSeconds_.push_back(warmupOffset);

    for (double start = warmupOffset; start < config_.durationSeconds;
         start += profile.chirpIntervalSeconds) {
      addChirp(mix, profile, start);
      events_.push_back({slot, start});
    }
  }

  // Scale to the configured loudness and convert to 16-bit, clipping anything past full scale.
  std::vector<std::int16_t> samples(totalSamples);
  for (std::size_t i = 0; i < totalSamples; ++i) {
    const double scaled = static_cast<double>(mix[i]) * config_.amplitude * kInt16FullScale;
    samples[i] = static_cast<std::int16_t>(std::clamp(scaled, -kInt16FullScale, kInt16FullScale));
  }
  return samples;
}
