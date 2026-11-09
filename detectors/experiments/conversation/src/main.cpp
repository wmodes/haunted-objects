/**
 * @file main.cpp
 * @brief Conversation experiment: generates a heat-driven conversation and renders it to a WAV.
 *
 * Generates a conversation between two detector slots, renders it with each slot's real voice,
 * writes a WAV, and prints a report (cast, heat curve, pace by heat) so the result can be judged
 * without listening. Play the file with `afplay` on macOS.
 */
#include <algorithm>
#include <cstdio>
#include <limits>
#include <vector>

#include "ChirpRenderer.h"
#include "ConversationConfig.h"
#include "ConversationGenerator.h"
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

/** Prints the heat curve every `stepSeconds`, as a row of marks per tenth of heat. */
void printHeatProfile(const Conversation& conversation, double durationSeconds,
                      double stepSeconds) {
  std::printf("heat over time (each # is 0.1):\n");
  for (double t = 0.0; t < durationSeconds; t += stepSeconds) {
    const double heat = conversation.heat.heatAt(t);
    const int marks = static_cast<int>(heat * 10.0 + 0.5);
    std::printf("  %5.1f s  %.2f  %.*s\n", t, heat, marks, "##########");
  }
}

/** Pace of the conversation split by how hot it was when each turn started. */
void printPaceByHeat(const Conversation& conversation) {
  constexpr int kBands = 3;
  const char* names[kBands] = {"cold (<0.33)", "warm (<0.66)", "hot  (>=0.66)"};
  double sumSeconds[kBands] = {0, 0, 0};
  double minSeconds[kBands] = {std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max()};
  int count[kBands] = {0, 0, 0};

  // The final turn has no successor, so it has no interval to measure.
  for (std::size_t i = 0; i + 1 < conversation.turns.size(); ++i) {
    const ConversationTurn& turn = conversation.turns[i];
    const int band = turn.heat < 1.0 / 3.0 ? 0 : (turn.heat < 2.0 / 3.0 ? 1 : 2);
    sumSeconds[band] += turn.intervalToNextSeconds;
    minSeconds[band] = std::min(minSeconds[band], turn.intervalToNextSeconds);
    ++count[band];
  }

  std::printf("pace by heat (time from one turn's start to the next):\n");
  for (int band = 0; band < kBands; ++band) {
    if (count[band] == 0) {
      std::printf("  %-14s no turns\n", names[band]);
    } else {
      std::printf("  %-14s %3d turns, mean %6.0f ms, shortest %5.0f ms\n", names[band],
                  count[band], sumSeconds[band] / count[band] * 1000.0, minSeconds[band] * 1000.0);
    }
  }
}

}  // namespace

int main() {
  // Built once, then handed to the parts that need it.
  constexpr ConversationConfig config;

  const ConversationGenerator generator(config, kDetectorProfiles);
  const Conversation conversation = generator.generate();

  const ChirpRenderer renderer(config.audio, kDetectorProfiles, kDetectorProfileCount);
  const std::vector<std::int16_t> samples =
      renderer.render(conversation.chirps, config.durationSeconds);

  if (!WavFileWriter::write(config.audio.outputFilePath, samples, config.audio.sampleRateHertz)) {
    std::fprintf(stderr, "could not write %s\n", config.audio.outputFilePath);
    return 1;
  }

  std::printf("wrote %s: %.1f s, %u Hz, %s wave, seed %u\n\n", config.audio.outputFilePath,
              config.durationSeconds, config.audio.sampleRateHertz,
              waveformName(config.audio.waveform), config.randomSeed);

  std::printf("cast (temperament drawn for this conversation):\n");
  for (const Participant& participant : conversation.cast) {
    const DetectorProfile& profile = kDetectorProfiles[participant.slot - 1];
    int turns = 0;
    for (const ConversationTurn& turn : conversation.turns) {
      turns += (turn.slot == participant.slot) ? 1 : 0;
    }
    std::printf("  slot %2u  %4u Hz %5.1f ms  talkativeness %.2f  wordiness %.2f  -> %d turns\n",
                participant.slot, profile.chirpFrequencyHertz, profile.chirpLengthMilliseconds,
                participant.talkativeness, participant.wordiness, turns);
  }

  std::printf("\nheat curve: peak %.2f at %.1f s, width %.1f s\n", conversation.heat.peakHeat(),
              conversation.heat.peakTimeSeconds(), conversation.heat.widthSeconds());
  std::printf("%zu turns, %zu chirps\n\n", conversation.turns.size(), conversation.chirps.size());

  printHeatProfile(conversation, config.durationSeconds, 5.0);
  std::printf("\n");
  printPaceByHeat(conversation);

  std::printf("\nfirst turns:\n");
  const std::size_t shown = std::min<std::size_t>(conversation.turns.size(), 20);
  for (std::size_t i = 0; i < shown; ++i) {
    const ConversationTurn& turn = conversation.turns[i];
    std::printf("  %7.3f s  slot %2u  %u chirp%s  heat %.2f  next in %5.0f ms\n", turn.startSeconds,
                turn.slot, turn.chirpCount, turn.chirpCount == 1 ? " " : "s", turn.heat,
                turn.intervalToNextSeconds * 1000.0);
  }

  return 0;
}
