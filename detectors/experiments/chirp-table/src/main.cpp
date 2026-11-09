/**
 * @file main.cpp
 * @brief Chirp-table experiment: renders slots of the detector profile table to a WAV file.
 *
 * Reads the real profile table, renders the configured slots as chirps with random warmup offsets,
 * writes a WAV, and prints a timeline so the result can be checked without listening. Play the
 * file with `afplay` on macOS.
 */
#include <algorithm>
#include <cstdio>
#include <vector>

#include "ChirpMixer.h"
#include "ChirpTableConfig.h"
#include "DetectorProfiles.h"
#include "WavFileWriter.h"

namespace {

const char* waveformName(Waveform waveform) {
  switch (waveform) {
    case Waveform::Sine:
      return "sine";
    case Waveform::Square:
      return "square";
    case Waveform::Sawtooth:
      return "sawtooth";
  }
  return "unknown";
}

}  // namespace

int main() {
  // Built once, then handed to the parts that need it.
  constexpr ChirpTableConfig config;

  ChirpMixer mixer(config, kDetectorProfiles, kDetectorProfileCount);
  const std::vector<std::int16_t> samples = mixer.render();

  if (!WavFileWriter::write(config.outputFilePath, samples, config.sampleRateHertz)) {
    std::fprintf(stderr, "could not write %s\n", config.outputFilePath);
    return 1;
  }

  std::printf("wrote %s: %.1f s, %u Hz, %s wave, seed %u\n\n", config.outputFilePath,
              config.durationSeconds, config.sampleRateHertz, waveformName(config.waveform),
              config.randomSeed);

  std::printf("slot  model                   length   pitch  interval  warmup\n");
  for (unsigned slot = config.firstSlot; slot <= config.lastSlot; ++slot) {
    const DetectorProfile& profile = kDetectorProfiles[slot - 1];
    const double warmup = mixer.warmupOffsetsSeconds()[slot - config.firstSlot];
    std::printf("%4u  %-22s %5.1f ms %5u Hz %6.1f s %6.1f s\n", profile.slot, profile.model,
                profile.chirpLengthMilliseconds, profile.chirpFrequencyHertz,
                profile.chirpIntervalSeconds, warmup);
  }

  // Timeline in playback order, so the run reads like what you will hear.
  std::vector<ChirpEvent> timeline = mixer.events();
  std::sort(timeline.begin(), timeline.end(), [](const ChirpEvent& a, const ChirpEvent& b) {
    return a.startSeconds < b.startSeconds;
  });
  std::printf("\n%zu chirps:\n", timeline.size());
  for (const ChirpEvent& event : timeline) {
    std::printf("  %7.2f s  slot %u\n", event.startSeconds, event.slot);
  }

  return 0;
}
