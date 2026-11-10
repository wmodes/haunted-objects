/**
 * @file main.cpp
 * @brief Conversation experiment: generates a heat-driven conversation and renders it to a WAV.
 *
 * Generates one conversation between two detector slots, renders it with each slot's real voice,
 * writes a WAV, and prints a report. It then generates many more conversations (different seeds)
 * and prints summary statistics, so the model can be judged against real turn-taking figures
 * without relying on one lucky run. Play the file with `afplay` on macOS.
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "ChirpRenderer.h"
#include "ConversationConfig.h"
#include "ConversationGenerator.h"
#include "DetectorProfiles.h"
#include "WavFileWriter.h"

namespace {

constexpr int kHeatBands = 3;
const char* const kHeatBandNames[kHeatBands] = {"cold (<0.33)", "warm (<0.66)", "hot (>=0.66)"};

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

int heatBand(double heat) {
  return heat < 1.0 / 3.0 ? 0 : (heat < 2.0 / 3.0 ? 1 : 2);
}

/** The value `fraction` (0..1) of the way through the sorted copy of `values`; 0 if empty. */
double percentile(std::vector<double> values, double fraction) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  return values[static_cast<std::size_t>(fraction * (values.size() - 1) + 0.5)];
}

double mean(const std::vector<double>& values) {
  if (values.empty()) {
    return 0.0;
  }
  double sum = 0.0;
  for (double value : values) {
    sum += value;
  }
  return sum / values.size();
}

/** Pearson correlation of two equal-length series; 0 if there is nothing to correlate. */
double correlation(const std::vector<double>& x, const std::vector<double>& y) {
  if (x.size() < 2 || x.size() != y.size()) {
    return 0.0;
  }
  const double meanX = mean(x);
  const double meanY = mean(y);
  double covariance = 0.0;
  double varianceX = 0.0;
  double varianceY = 0.0;
  for (std::size_t i = 0; i < x.size(); ++i) {
    covariance += (x[i] - meanX) * (y[i] - meanY);
    varianceX += (x[i] - meanX) * (x[i] - meanX);
    varianceY += (y[i] - meanY) * (y[i] - meanY);
  }
  const double denominator = std::sqrt(varianceX * varianceY);
  return denominator > 0.0 ? covariance / denominator : 0.0;
}

/** What is measured about conversations, pooled over as many as are accumulated. */
struct Statistics {
  std::size_t conversations = 0;
  std::size_t turns = 0;
  std::size_t doubleChirpTurns = 0;
  std::vector<double> spacingSeconds;  // turn start to next turn start
  std::size_t bandTurns[kHeatBands] = {0, 0, 0};
  std::size_t bandOverlaps[kHeatBands] = {0, 0, 0};
  std::vector<double> bandSpacingSeconds[kHeatBands];
  std::vector<double> bandOrdinaryGapSeconds[kHeatBands];
  std::vector<double> sameSpeakerGapSeconds;    // ordinary pauses when the floor is kept
  std::vector<double> speakerChangeGapSeconds;  // ordinary pauses when the floor passes
  std::vector<double> ordinaryGapSeconds;       // every ordinary pause
  std::vector<double> earlierLogGap;            // log of an ordinary pause ...
  std::vector<double> laterLogGap;              // ... and of the ordinary pause right after it
  std::size_t lapsesBeforePeak = 0;
  std::size_t lapsesAfterPeak = 0;
  // The cool two-speaker opening (heat < 0.33), ordinary turns only: the silence the ear hears
  // between one turn's last chirp and the next turn's first.
  std::vector<double> coolPauseSeconds;
  // The early part: cool and before the heat peak (the wind-down is cool too, but not early).
  std::vector<double> earlyPauseSeconds;
  // One entry per conversation, to check that length is independent of the pace.
  std::vector<double> durationsSeconds;
  std::vector<double> meanPauseSecondsPerConversation;
  std::vector<double> turnsPerConversation;
  // Newcomers: how many are present, how many actually pitch in, and how often.
  std::size_t newcomersPresentCount[4] = {0, 0, 0, 0};  // conversations with k newcomers present
  std::size_t newcomersPitchedCount[4] = {0, 0, 0, 0};  // ... in which k of them pitched in
  std::vector<double> pitchInsPerConversation;
  std::size_t pitchIns = 0;
  std::size_t pitchInJumpIns = 0;
  // Chirp density by heat: seconds spent in each band, and chirps heard in it.
  double bandSeconds[kHeatBands] = {0.0, 0.0, 0.0};
  double bandOriginalChirps[kHeatBands] = {0.0, 0.0, 0.0};
  double bandNewcomerChirps[kHeatBands] = {0.0, 0.0, 0.0};
};

void accumulate(Statistics& stats, const Conversation& conversation) {
  ++stats.conversations;
  const double peakTime = conversation.heat.peakTimeSeconds();
  bool previousWasOrdinary = false;
  double previousLogGap = 0.0;
  double ordinaryGapSum = 0.0;
  std::size_t ordinaryGapCount = 0;
  for (std::size_t i = 0; i < conversation.turns.size(); ++i) {
    const ConversationTurn& turn = conversation.turns[i];
    ++stats.turns;
    stats.doubleChirpTurns += (turn.chirpCount == 2) ? 1 : 0;

    const int band = heatBand(turn.heat);
    ++stats.bandTurns[band];
    stats.bandOverlaps[band] += turn.overlapsPrevious ? 1 : 0;
    stats.bandOriginalChirps[band] += turn.chirpCount;

    if (i + 1 < conversation.turns.size()) {  // the final turn has no successor
      stats.spacingSeconds.push_back(turn.intervalToNextSeconds);
      stats.bandSpacingSeconds[band].push_back(turn.intervalToNextSeconds);

      const ConversationTurn& next = conversation.turns[i + 1];
      if (band == 0 && !next.overlapsPrevious && !next.afterLapse) {
        stats.coolPauseSeconds.push_back(next.gapBeforeSeconds);
        if (turn.startSeconds < peakTime) {
          stats.earlyPauseSeconds.push_back(next.gapBeforeSeconds);
        }
      }
    }

    // An ordinary pause is neither a jump-in nor a lapse; those are measured separately.
    const bool ordinary = i > 0 && !turn.overlapsPrevious && !turn.afterLapse;
    if (ordinary) {
      (turn.followsSameSpeaker ? stats.sameSpeakerGapSeconds : stats.speakerChangeGapSeconds)
          .push_back(turn.gapBeforeSeconds);
      stats.ordinaryGapSeconds.push_back(turn.gapBeforeSeconds);
      stats.bandOrdinaryGapSeconds[band].push_back(turn.gapBeforeSeconds);
      ordinaryGapSum += turn.gapBeforeSeconds;
      ++ordinaryGapCount;
      const double logGap = std::log(turn.gapBeforeSeconds);
      if (previousWasOrdinary) {
        stats.earlierLogGap.push_back(previousLogGap);
        stats.laterLogGap.push_back(logGap);
      }
      previousLogGap = logGap;
    }
    previousWasOrdinary = ordinary;

    if (turn.afterLapse) {
      (turn.startSeconds < peakTime ? stats.lapsesBeforePeak : stats.lapsesAfterPeak) += 1;
    }
  }

  stats.durationsSeconds.push_back(conversation.durationSeconds);
  stats.meanPauseSecondsPerConversation.push_back(
      ordinaryGapCount > 0 ? ordinaryGapSum / ordinaryGapCount : 0.0);
  stats.turnsPerConversation.push_back(static_cast<double>(conversation.turns.size()));

  // Time spent in each heat band, sampled every tenth of a second.
  constexpr double kSampleSeconds = 0.1;
  for (double t = 0.0; t < conversation.durationSeconds; t += kSampleSeconds) {
    stats.bandSeconds[heatBand(conversation.heat.heatAt(t))] += kSampleSeconds;
  }

  // Newcomers: present, pitched in, and where their chirps fall.
  std::size_t present = 0;
  std::vector<unsigned> pitchedSlots;
  for (const Participant& participant : conversation.cast) {
    present += participant.isOriginal ? 0 : 1;
  }
  for (const Interjection& pitchIn : conversation.interjections) {
    ++stats.pitchIns;
    stats.pitchInJumpIns += pitchIn.jumpedIn ? 1 : 0;
    stats.bandNewcomerChirps[heatBand(conversation.heat.heatAt(pitchIn.startSeconds))] += 1.0;
    if (std::find(pitchedSlots.begin(), pitchedSlots.end(), pitchIn.slot) == pitchedSlots.end()) {
      pitchedSlots.push_back(pitchIn.slot);
    }
  }
  ++stats.newcomersPresentCount[std::min<std::size_t>(present, 3)];
  ++stats.newcomersPitchedCount[std::min<std::size_t>(pitchedSlots.size(), 3)];
  stats.pitchInsPerConversation.push_back(static_cast<double>(conversation.interjections.size()));
}

void printStatistics(const Statistics& stats) {
  const double conversations = static_cast<double>(stats.conversations);
  std::printf("  %zu turns over %zu conversations; two-chirp turns: %.1f%%\n", stats.turns,
              stats.conversations,
              100.0 * stats.doubleChirpTurns / std::max<std::size_t>(stats.turns, 1));
  std::printf("  conversation length: shortest / median / longest %.0f / %.0f / %.0f s; "
              "turns per conversation (median) %.0f\n",
              percentile(stats.durationsSeconds, 0.0), percentile(stats.durationsSeconds, 0.5),
              percentile(stats.durationsSeconds, 1.0),
              percentile(stats.turnsPerConversation, 0.5));
  std::printf("  length vs pace: correlation of length with mean pause %.2f "
              "(near 0 = a longer conversation has more turns, not slower ones)\n",
              correlation(stats.durationsSeconds, stats.meanPauseSecondsPerConversation));
  std::printf("  turn start to next turn start: median %.0f ms, quartiles %.0f / %.0f ms\n",
              percentile(stats.spacingSeconds, 0.5) * 1000.0,
              percentile(stats.spacingSeconds, 0.25) * 1000.0,
              percentile(stats.spacingSeconds, 0.75) * 1000.0);
  std::printf("  by heat (talking over = landing within a few tens of ms, heard as a collision):\n");
  for (int band = 0; band < kHeatBands; ++band) {
    std::size_t under100 = 0;
    std::size_t under30 = 0;
    for (double gap : stats.bandOrdinaryGapSeconds[band]) {
      under100 += gap < 0.100 ? 1 : 0;
      under30 += gap < 0.030 ? 1 : 0;
    }
    const double ordinary =
        static_cast<double>(std::max<std::size_t>(stats.bandOrdinaryGapSeconds[band].size(), 1));
    std::printf("    %-14s %6zu turns   median spacing %5.0f ms   talking over %4.1f%%   "
                "ordinary pauses under 100 ms %4.1f%%, under 30 ms %4.1f%%\n",
                kHeatBandNames[band], stats.bandTurns[band],
                percentile(stats.bandSpacingSeconds[band], 0.5) * 1000.0,
                100.0 * stats.bandOverlaps[band] / std::max<std::size_t>(stats.bandTurns[band], 1),
                100.0 * under100 / ordinary, 100.0 * under30 / ordinary);
  }

  // How dense each heat band is, and who fills it: the heated stretch should be sparse for the
  // two original speakers, with room for newcomers.
  std::printf("  chirps per second by heat:   original speakers / newcomers / everyone\n");
  for (int band = 0; band < kHeatBands; ++band) {
    const double seconds = std::max(stats.bandSeconds[band], 1e-9);
    const double original = stats.bandOriginalChirps[band] / seconds;
    const double newcomer = stats.bandNewcomerChirps[band] / seconds;
    std::printf("    %-14s %.2f / %.2f / %.2f\n", kHeatBandNames[band], original, newcomer,
                original + newcomer);
  }
  const double secondsInAnyBand =
      std::max(stats.bandSeconds[0] + stats.bandSeconds[1] + stats.bandSeconds[2], 1e-9);
  std::printf("  share of the time at each heat:   %.0f%% cold / %.0f%% warm / %.0f%% hot\n",
              100.0 * stats.bandSeconds[0] / secondsInAnyBand,
              100.0 * stats.bandSeconds[1] / secondsInAnyBand,
              100.0 * stats.bandSeconds[2] / secondsInAnyBand);
  const double all = static_cast<double>(std::max<std::size_t>(stats.conversations, 1));
  std::printf("  newcomers present (0/1/2/3):   %2.0f%% / %2.0f%% / %2.0f%% / %2.0f%%\n",
              100.0 * stats.newcomersPresentCount[0] / all,
              100.0 * stats.newcomersPresentCount[1] / all,
              100.0 * stats.newcomersPresentCount[2] / all,
              100.0 * stats.newcomersPresentCount[3] / all);
  std::printf("  newcomers who pitched in (0/1/2/3): %2.0f%% / %2.0f%% / %2.0f%% / %2.0f%%\n",
              100.0 * stats.newcomersPitchedCount[0] / all,
              100.0 * stats.newcomersPitchedCount[1] / all,
              100.0 * stats.newcomersPitchedCount[2] / all,
              100.0 * stats.newcomersPitchedCount[3] / all);
  std::printf("  newcomer pitch-ins per conversation: median %.0f, mean %.1f; landing on top of "
              "a turn: %.0f%%\n",
              percentile(stats.pitchInsPerConversation, 0.5), mean(stats.pitchInsPerConversation),
              100.0 * stats.pitchInJumpIns / std::max<std::size_t>(stats.pitchIns, 1));

  // The rhythm of the pauses: how varied they are, and whether they come in streaks.
  const double p10 = percentile(stats.ordinaryGapSeconds, 0.10) * 1000.0;
  const double p50 = percentile(stats.ordinaryGapSeconds, 0.50) * 1000.0;
  const double p90 = percentile(stats.ordinaryGapSeconds, 0.90) * 1000.0;
  std::printf("  ordinary pauses: 10th / median / 90th percentile %.0f / %.0f / %.0f ms "
              "(90th is %.0fx the 10th)\n",
              p10, p50, p90, p10 > 0.0 ? p90 / p10 : 0.0);
  std::size_t bins[4] = {0, 0, 0, 0};
  for (double gap : stats.ordinaryGapSeconds) {
    const double ms = gap * 1000.0;
    bins[ms < 100.0 ? 0 : (ms < 400.0 ? 1 : (ms < 1000.0 ? 2 : 3))] += 1;
  }
  const double total = static_cast<double>(std::max<std::size_t>(stats.ordinaryGapSeconds.size(), 1));
  std::printf("  pause sizes: under 100 ms %.0f%%, 100-400 ms %.0f%%, 400-1000 ms %.0f%%, "
              "over 1 s %.0f%%\n",
              100.0 * bins[0] / total, 100.0 * bins[1] / total, 100.0 * bins[2] / total,
              100.0 * bins[3] / total);
  std::printf("  one pause predicts the next (correlation of log pauses; 0 = no rhythm): %.2f\n",
              correlation(stats.earlierLogGap, stats.laterLogGap));
  std::printf("  mean ordinary pause: same speaker %.0f ms, speaker change %.0f ms\n",
              mean(stats.sameSpeakerGapSeconds) * 1000.0,
              mean(stats.speakerChangeGapSeconds) * 1000.0);
  std::printf("  lapses per conversation: before the heat peak %.2f, after it %.2f\n",
              stats.lapsesBeforePeak / conversations, stats.lapsesAfterPeak / conversations);

  // The cool opening, as the ear hears it: silence between one chirp and the next.
  const std::vector<double>& cool = stats.coolPauseSeconds;
  std::printf("  cool opening (heat < 0.33, no jump-ins or lapses), silence between chirps:\n");
  std::printf("    min / 5th / 50th / 95th / max: %.0f / %.0f / %.0f / %.0f / %.0f ms\n",
              percentile(cool, 0.0) * 1000.0, percentile(cool, 0.05) * 1000.0,
              percentile(cool, 0.50) * 1000.0, percentile(cool, 0.95) * 1000.0,
              percentile(cool, 1.0) * 1000.0);
  // The early part only: cool and before the heat peak. Long pauses here should stay modest.
  const std::vector<double>& early = stats.earlyPauseSeconds;
  std::size_t earlyOver2 = 0;
  std::size_t earlyOver3 = 0;
  for (double pause : early) {
    earlyOver2 += pause > 2.0 ? 1 : 0;
    earlyOver3 += pause > 3.0 ? 1 : 0;
  }
  const double earlyCount = static_cast<double>(std::max<std::size_t>(early.size(), 1));
  std::printf("  early part (cool and before the heat peak), silence between chirps:\n");
  std::printf("    50th / 90th / 95th / 99th / max: %.0f / %.0f / %.0f / %.0f / %.0f ms\n",
              percentile(early, 0.50) * 1000.0, percentile(early, 0.90) * 1000.0,
              percentile(early, 0.95) * 1000.0, percentile(early, 0.99) * 1000.0,
              percentile(early, 1.0) * 1000.0);
  std::printf("    over 2 s: %.1f%%   over 3 s: %.1f%%\n", 100.0 * earlyOver2 / earlyCount,
              100.0 * earlyOver3 / earlyCount);
  std::printf("    mean %.0f ms, so about %.1f turns per second\n", mean(cool) * 1000.0,
              mean(cool) > 0.0 ? 1.0 / mean(cool) : 0.0);
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

}  // namespace

int main(int argc, char** argv) {
  // Built once, then handed to the parts that need it. An optional first argument overrides the
  // seed, to hear a different conversation without rebuilding.
  ConversationConfig config;
  if (argc > 1) {
    config.randomSeed = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
  }

  const ConversationGenerator generator(config, kDetectorProfiles);
  const Conversation conversation = generator.generate();

  const ChirpRenderer renderer(config.audio, kDetectorProfiles, kDetectorProfileCount);
  const std::vector<std::int16_t> samples =
      renderer.render(conversation.chirps, conversation.durationSeconds);

  if (!WavFileWriter::write(config.audio.outputFilePath, samples, config.audio.sampleRateHertz)) {
    std::fprintf(stderr, "could not write %s\n", config.audio.outputFilePath);
    return 1;
  }

  std::printf("wrote %s: %.1f s (drawn from %.0f-%.0f s), %u Hz, %s wave, seed %u\n\n",
              config.audio.outputFilePath, conversation.durationSeconds,
              config.durationSeconds.min, config.durationSeconds.max,
              config.audio.sampleRateHertz,
              waveformName(config.audio.waveform), config.randomSeed);

  std::printf("cast (temperament drawn for this conversation):\n");
  for (const Participant& participant : conversation.cast) {
    const DetectorProfile& profile = kDetectorProfiles[participant.slot - 1];
    if (participant.isOriginal) {
      int turns = 0;
      for (const ConversationTurn& turn : conversation.turns) {
        turns += (turn.slot == participant.slot) ? 1 : 0;
      }
      std::printf("  slot %2u  %4u Hz %5.1f ms  original speaker  talkativeness %.2f  "
                  "wordiness %.2f  -> %d turns\n",
                  participant.slot, profile.chirpFrequencyHertz, profile.chirpLengthMilliseconds,
                  participant.talkativeness, participant.wordiness, turns);
    } else {
      int pitchIns = 0;
      for (const Interjection& pitchIn : conversation.interjections) {
        pitchIns += (pitchIn.slot == participant.slot) ? 1 : 0;
      }
      std::printf("  slot %2u  %4u Hz %5.1f ms  newcomer          talkativeness %.2f  "
                  "-> %d pitch-ins\n",
                  participant.slot, profile.chirpFrequencyHertz, profile.chirpLengthMilliseconds,
                  participant.talkativeness, pitchIns);
    }
  }

  std::printf("\nheat curve: peak %.2f at %.1f s, width %.1f s\n", conversation.heat.peakHeat(),
              conversation.heat.peakTimeSeconds(), conversation.heat.widthSeconds());
  std::printf("%zu turns, %zu chirps\n", conversation.turns.size(), conversation.chirps.size());

  // Chirps that start within a chirp's length (plus the analysis gap) of another overlap in the
  // audio, so a burst detector counts them as one. Counted here to reconcile it with the WAV.
  std::vector<double> chirpStarts;
  for (const ChirpEvent& chirp : conversation.chirps) {
    chirpStarts.push_back(chirp.startSeconds);
  }
  std::sort(chirpStarts.begin(), chirpStarts.end());
  std::size_t colliding = 0;
  for (std::size_t i = 1; i < chirpStarts.size(); ++i) {
    colliding += (chirpStarts[i] - chirpStarts[i - 1] < 0.0143) ? 1 : 0;
  }
  std::printf("%zu chirps start within 14 ms of the one before (they overlap in the audio)\n\n",
              colliding);
  // About eighteen rows, whatever the length.
  printHeatProfile(conversation, conversation.durationSeconds, conversation.durationSeconds / 18.0);

  std::printf("\nthis conversation:\n");
  Statistics single;
  accumulate(single, conversation);
  printStatistics(single);

  std::printf("\nlapses in this conversation (after a long silence):");
  bool anyLapse = false;
  for (const ConversationTurn& turn : conversation.turns) {
    if (turn.afterLapse) {
      std::printf(" %.1f s", turn.startSeconds);
      anyLapse = true;
    }
  }
  std::printf("%s\n", anyLapse ? "" : " none");

  std::printf("\nfirst turns (difficulty = how hard that turn is to answer; the pause before the\n"
              "next turn follows it):\n");
  const std::size_t shown = std::min<std::size_t>(conversation.turns.size(), 30);
  for (std::size_t i = 0; i < shown; ++i) {
    const ConversationTurn& turn = conversation.turns[i];
    std::printf("  %7.3f s  slot %2u  %u chirp%s  difficulty %.2f  pause before %5.0f ms%s%s\n",
                turn.startSeconds, turn.slot, turn.chirpCount, turn.chirpCount == 1 ? " " : "s",
                turn.difficulty, turn.gapBeforeSeconds * 1000.0,
                turn.followsSameSpeaker ? "  (same speaker)" : "",
                turn.overlapsPrevious ? "  (talking over)" : "");
  }

  std::printf("\nnewcomer pitch-ins in this conversation (time, slot, where it landed):\n");
  for (const Interjection& pitchIn : conversation.interjections) {
    std::printf("  %7.2f s  slot %2u  %s\n", pitchIn.startSeconds, pitchIn.slot,
                pitchIn.jumpedIn ? "on top of a turn" : "in a silence");
  }
  if (conversation.interjections.empty()) {
    std::printf("  none\n");
  }

  // The same model over many seeds, to see its averages and not one run's luck.
  Statistics pooled;
  for (unsigned i = 0; i < config.statisticsConversationCount; ++i) {
    ConversationConfig variant = config;
    variant.randomSeed = config.randomSeed + i;
    accumulate(pooled, ConversationGenerator(variant, kDetectorProfiles).generate());
  }
  std::printf("\nacross %u conversations (seeds %u onward):\n", config.statisticsConversationCount,
              config.randomSeed);
  printStatistics(pooled);

  return 0;
}
