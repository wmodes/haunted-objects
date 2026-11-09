/**
 * @file ChirpRenderer.cpp
 * @brief Implementation of ChirpRenderer. See ChirpRenderer.h.
 */
#include "ChirpRenderer.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

/** Full-scale value of a signed 16-bit sample. */
constexpr double kInt16FullScale = 32767.0;

constexpr double kMillisecondsPerSecond = 1000.0;

}  // namespace

ChirpRenderer::ChirpRenderer(const AudioConfig& audio, const DetectorProfile* profiles,
                             std::size_t profileCount)
    : audio_(audio), profiles_(profiles), profileCount_(profileCount) {}

float ChirpRenderer::waveformValue(double phase) const {
  switch (audio_.waveform) {
    case Waveform::Sine:
      return static_cast<float>(std::sin(2.0 * kPi * phase));
    case Waveform::Square:
      return phase < 0.5 ? 1.0f : -1.0f;
    case Waveform::Sawtooth:
      return static_cast<float>(2.0 * phase - 1.0);
  }
  return 0.0f;
}

void ChirpRenderer::addChirp(std::vector<float>& mix, const DetectorProfile& profile,
                             double startSeconds) const {
  const double rate = audio_.sampleRateHertz;
  const std::size_t startSample = static_cast<std::size_t>(std::llround(startSeconds * rate));
  const std::size_t lengthSamples = static_cast<std::size_t>(
      std::llround(profile.chirpLengthMilliseconds / kMillisecondsPerSecond * rate));

  // The fade is capped at half the chirp so the fade-in and fade-out never overlap.
  const std::size_t fadeSamples = std::min(
      static_cast<std::size_t>(
          std::llround(audio_.fadeMilliseconds / kMillisecondsPerSecond * rate)),
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

std::vector<std::int16_t> ChirpRenderer::render(const std::vector<ChirpEvent>& chirps,
                                                double durationSeconds) const {
  const std::size_t totalSamples =
      static_cast<std::size_t>(std::llround(durationSeconds * audio_.sampleRateHertz));
  std::vector<float> mix(totalSamples, 0.0f);

  for (const ChirpEvent& chirp : chirps) {
    if (chirp.slot >= 1 && chirp.slot <= profileCount_ && chirp.startSeconds < durationSeconds) {
      addChirp(mix, profiles_[chirp.slot - 1], chirp.startSeconds);
    }
  }

  // Scale to the configured loudness and convert to 16-bit, clipping anything past full scale.
  std::vector<std::int16_t> samples(totalSamples);
  for (std::size_t i = 0; i < totalSamples; ++i) {
    const double scaled = static_cast<double>(mix[i]) * audio_.amplitude * kInt16FullScale;
    samples[i] = static_cast<std::int16_t>(std::clamp(scaled, -kInt16FullScale, kInt16FullScale));
  }
  return samples;
}
