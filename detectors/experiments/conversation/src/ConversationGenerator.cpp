/**
 * @file ConversationGenerator.cpp
 * @brief Implementation of ConversationGenerator. See ConversationGenerator.h.
 */
#include "ConversationGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace {

constexpr double kMillisecondsPerSecond = 1000.0;

/**
 * How many times a bounded draw is retried before it is simply clamped. Redrawing keeps the shape
 * of the distribution (no pile-up at the bounds); the clamp is only a backstop.
 */
constexpr int kBoundedDrawAttempts = 32;

double drawFrom(const Range& range, std::mt19937& rng) {
  return std::uniform_real_distribution<double>(range.min, range.max)(rng);
}

/** Position `amount` (0..1) of the way from `from` to `to`. */
double blend(double from, double to, double amount) {
  return from + (to - from) * amount;
}

/**
 * Position `amount` (0..1) of the way from `from` to `to` on a log scale, so equal steps in
 * `amount` multiply the value by equal factors. Right for timing, where 300 ms to 1800 ms is a
 * six-fold range and the middle should be about 735 ms, not 1050 ms.
 */
double blendLog(double from, double to, double amount) {
  return std::exp(blend(std::log(from), std::log(to), amount));
}

/**
 * A skewed draw: log-normal with the given median and spread (sigma). Mostly values near the
 * median, with a long tail of larger ones, like real speech timing.
 */
double drawSkewed(double median, double spread, std::mt19937& rng) {
  return std::lognormal_distribution<double>(std::log(median), spread)(rng);
}

/** Smooth 0-to-1 ramp: 0 at x <= 0, 1 at x >= 1, no sharp corners. */
double smoothstep(double x) {
  const double clamped = std::clamp(x, 0.0, 1.0);
  return clamped * clamped * (3.0 - 2.0 * clamped);
}

/**
 * How hard the next turn is to answer, given how hard the one before it was, and never harder than
 * `cap`. Usually a small drift from the previous level; now and then a topic shift to a fresh
 * level, biased toward easy and spread over the range the cap allows.
 */
double drawDifficulty(const DifficultyConfig& config, double previous, double cap,
                      std::mt19937& rng) {
  if (std::bernoulli_distribution(config.topicShiftProbability)(rng)) {
    return cap *
           std::pow(std::uniform_real_distribution<double>(0.0, 1.0)(rng), config.topicShiftSkew);
  }
  return std::clamp(previous + std::normal_distribution<double>(0.0, config.driftSpread)(rng),
                    0.0, cap);
}

}  // namespace

ConversationGenerator::ConversationGenerator(const ConversationConfig& config,
                                             const DetectorProfile* profiles)
    : config_(config), profiles_(profiles) {}

Conversation ConversationGenerator::generate() const {
  const TimingConfig& timing = config_.timing;
  const TemperamentConfig& temperament = config_.temperament;
  std::mt19937 rng(config_.randomSeed);

  // One generator, consumed in a fixed order, so a seed fixes the whole conversation. The length
  // is drawn first, on its own, so it models how long the conversation lasts and nothing about
  // how quickly the turns come.
  const double durationSeconds = drawFrom(config_.durationSeconds, rng);
  HeatCurve heat = HeatCurve::draw(config_.heat, durationSeconds, rng);

  std::vector<Participant> cast;
  for (unsigned slot : {config_.firstCastSlot, config_.secondCastSlot}) {
    cast.push_back({slot, drawFrom(config_.temperament.talkativeness, rng),
                    drawFrom(config_.temperament.wordiness, rng), true});
  }

  // Talkativeness is a relative weight: the next speaker is drawn in proportion to it. The
  // current speaker stays in the draw, so a chatty slot can speak several turns in a row.
  std::vector<double> weights;
  for (const Participant& participant : cast) {
    weights.push_back(participant.talkativeness);
  }
  std::discrete_distribution<std::size_t> pickSpeaker(weights.begin(), weights.end());

  Conversation conversation{durationSeconds, cast, heat, {}, {}, {}};

  // When each cast member's last chirp ended, so a slot never starts a chirp on top of its own.
  std::vector<double> lastChirpEndSeconds(cast.size(),
                                          -std::numeric_limits<double>::infinity());

  // How the turn about to be placed was reached; the first turn simply opens the conversation.
  std::size_t speaker = pickSpeaker(rng);
  double nextStartSeconds = 0.0;
  double gapBeforeSeconds = 0.0;
  bool followsSameSpeaker = false;
  bool overlapsPrevious = false;
  bool afterLapse = false;
  // How hard the turn being answered was; the opening answers small talk.
  double demand = config_.difficulty.openingDifficulty;

  while (true) {
    const Participant& participant = cast[speaker];
    const double chirpLengthSeconds =
        profiles_[participant.slot - 1].chirpLengthMilliseconds / kMillisecondsPerSecond;

    const double startSeconds = std::max(
        nextStartSeconds,
        lastChirpEndSeconds[speaker] + timing.minSameSlotGapMilliseconds / kMillisecondsPerSecond);
    if (startSeconds >= durationSeconds) {
      break;
    }
    const double heatNow = heat.heatAt(startSeconds);

    // How far into the heated pacing the conversation is: 0 in the cool opening, 1 at full heat.
    const double heated = smoothstep((heatNow - timing.heatedStartHeat) /
                                     (timing.heatedFullHeat - timing.heatedStartHeat));

    // A turn is one chirp, or rarely two: someone adding on to what they just said. The chance
    // grows with heat and with the slot's wordiness.
    const double doubleChance =
        temperament.maxDoubleChirpProbability * participant.wordiness *
        blend(temperament.coldDoubleChirpFraction, 1.0, heatNow);
    const unsigned chirpCount = std::bernoulli_distribution(doubleChance)(rng) ? 2u : 1u;

    // The silence inside a two-chirp turn ("Well... part time") grows with how hard the answered
    // turn was.
    const double innerSpacingMilliseconds =
        drawSkewed(blendLog(timing.doubleChirpSpacingEasyMilliseconds,
                            timing.doubleChirpSpacingHardMilliseconds, demand),
                   timing.doubleChirpSpacingSpread, rng);
    for (unsigned i = 0; i < chirpCount; ++i) {
      const double chirpStartSeconds =
          startSeconds + i * innerSpacingMilliseconds / kMillisecondsPerSecond;
      if (chirpStartSeconds >= durationSeconds) {
        break;
      }
      conversation.chirps.push_back({participant.slot, chirpStartSeconds});
      lastChirpEndSeconds[speaker] = chirpStartSeconds + chirpLengthSeconds;
    }
    // The silence before the next turn is counted from this turn's last chirp.
    const double lastChirpOffsetMilliseconds = (chirpCount == 2) ? innerSpacingMilliseconds : 0.0;

    // How hard this turn is to answer. It sets the pause before the reply. Early talk is light:
    // before the heat peak the hardest topics wait until heat builds. After the peak there is no
    // cap, so the wind-down keeps its long pauses.
    const bool beforePeak = startSeconds < heat.peakTimeSeconds();
    const double difficultyCap =
        beforePeak ? blend(config_.difficulty.coolMaxDifficulty, 1.0, heated) : 1.0;
    const double difficulty = drawDifficulty(config_.difficulty, demand, difficultyCap, rng);

    // Who speaks next, and how they come in.
    const std::size_t nextSpeaker = pickSpeaker(rng);
    const bool sameSpeakerNext = nextSpeaker == speaker;

    // Lapses belong to the wind-down: only after the peak, and more likely the cooler it is.
    const double windDown =
        smoothstep((startSeconds - heat.peakTimeSeconds()) /
                   std::max(durationSeconds - heat.peakTimeSeconds(), 1e-9));
    const double lapseChance = timing.lapseMaxProbability * (1.0 - heatNow) * windDown;

    double gapMilliseconds;
    bool overlapNext = false;
    bool lapseNext = false;
    double intervalMilliseconds;

    // Jump-ins only appear as heat builds: none in the cool opening.
    const double overlapChance =
        timing.hotOverlapProbability *
        smoothstep((heatNow - timing.overlapStartHeat) /
                   (timing.overlapFullHeat - timing.overlapStartHeat));
    if (!sameSpeakerNext && std::bernoulli_distribution(overlapChance)(rng)) {
      // Jumping in: the next speaker starts almost on top of this turn's start.
      overlapNext = true;
      intervalMilliseconds = drawFrom(timing.overlapOnsetMilliseconds, rng);
      gapMilliseconds = intervalMilliseconds;
    } else if (std::bernoulli_distribution(lapseChance)(rng)) {
      lapseNext = true;
      gapMilliseconds = drawFrom(timing.lapseSeconds, rng) * kMillisecondsPerSecond;
      intervalMilliseconds = lastChirpOffsetMilliseconds + gapMilliseconds;
    } else {
      // An ordinary silence: short for something easy, long for something hard to answer, and a
      // little shorter when heated. A slot keeping the floor takes longer, as if still thinking.
      // It is redrawn until it falls inside the hard bounds, so there are no outliers.
      // The heated pacing ramps in only as heat builds, so the cool opening is untouched.
      const double medianMilliseconds =
          blendLog(timing.easyGapMedianMilliseconds, timing.hardGapMedianMilliseconds, difficulty) *
          blend(1.0, timing.hotThinkingScale, heated);
      const double extraMilliseconds =
          sameSpeakerNext ? timing.sameSpeakerExtraPauseMilliseconds : 0.0;
      // Both bounds slide with heat: the heated stretch is sparse, so its floor is higher and its
      // ceiling is higher too, leaving room for newcomers to pitch in.
      const double floorMilliseconds = blend(timing.minOrdinaryGapMilliseconds,
                                             timing.hotMinOrdinaryGapMilliseconds, heated);
      const double ceilingMilliseconds = blend(timing.maxOrdinaryGapMilliseconds,
                                               timing.hotMaxOrdinaryGapMilliseconds, heated);
      gapMilliseconds = 0.0;
      for (int attempt = 0; attempt < kBoundedDrawAttempts; ++attempt) {
        gapMilliseconds = drawSkewed(medianMilliseconds, timing.gapSpread, rng) + extraMilliseconds;
        if (gapMilliseconds >= floorMilliseconds && gapMilliseconds <= ceilingMilliseconds) {
          break;
        }
      }
      gapMilliseconds = std::clamp(gapMilliseconds, floorMilliseconds, ceilingMilliseconds);
      intervalMilliseconds = lastChirpOffsetMilliseconds + gapMilliseconds;
    }

    // The floor only stops a jump-in from collapsing onto the same instant.
    intervalMilliseconds = std::max(intervalMilliseconds, timing.minTurnSpacingMilliseconds);
    const double intervalSeconds = intervalMilliseconds / kMillisecondsPerSecond;

    conversation.turns.push_back({participant.slot, startSeconds, chirpCount, heatNow, difficulty,
                                  demand, intervalSeconds, gapBeforeSeconds, followsSameSpeaker,
                                  overlapsPrevious, afterLapse,
                                  lastChirpOffsetMilliseconds / kMillisecondsPerSecond});

    // Set up the next turn.
    gapBeforeSeconds = gapMilliseconds / kMillisecondsPerSecond;
    followsSameSpeaker = sameSpeakerNext;
    overlapsPrevious = overlapNext;
    afterLapse = lapseNext;
    speaker = nextSpeaker;
    demand = difficulty;
    nextStartSeconds = startSeconds + intervalSeconds;
  }

  // The last turn has no successor inside the conversation.
  if (!conversation.turns.empty()) {
    conversation.turns.back().intervalToNextSeconds = 0.0;
  }

  // Newcomers: other slots that pitch in on a conversation they are not part of. Drawn after the
  // original exchange so the original speakers' turns do not depend on them.
  const NewcomerConfig& newcomers = config_.newcomers;
  std::vector<unsigned> pool;
  for (unsigned slot = newcomers.poolFirstSlot; slot <= newcomers.poolLastSlot; ++slot) {
    pool.push_back(slot);
  }
  std::discrete_distribution<std::size_t> pickCount(std::begin(newcomers.countWeights),
                                                    std::end(newcomers.countWeights));
  const std::size_t newcomerCount = std::min(pickCount(rng), pool.size());
  std::shuffle(pool.begin(), pool.end(), rng);

  std::vector<Participant> newcomerCast;
  for (std::size_t i = 0; i < newcomerCount; ++i) {
    // A newcomer has a talkativeness like anyone else; it scales its chance of pitching in.
    newcomerCast.push_back({pool[i], drawFrom(temperament.talkativeness, rng), 0.0, false});
  }
  for (const Participant& newcomer : newcomerCast) {
    conversation.cast.push_back(newcomer);
  }

  std::vector<double> lastPitchInSeconds(newcomerCast.size(),
                                         -std::numeric_limits<double>::infinity());
  for (std::size_t i = 0; i < conversation.turns.size(); ++i) {
    const ConversationTurn& turn = conversation.turns[i];

    // Nearby tables join as the conversation gets vivid, and drift off as it cools.
    const double heatGate = smoothstep((turn.heat - newcomers.startHeat) /
                                       (newcomers.fullHeat - newcomers.startHeat));
    if (heatGate <= 0.0) {
      continue;
    }

    // The silence after this turn's last chirp, until the next original turn begins.
    const double silenceStartSeconds = turn.startSeconds + turn.lastChirpOffsetSeconds;
    const double silenceEndSeconds = (i + 1 < conversation.turns.size())
                                         ? conversation.turns[i + 1].startSeconds
                                         : durationSeconds;

    for (std::size_t n = 0; n < newcomerCast.size(); ++n) {
      const Participant& newcomer = newcomerCast[n];

      // The chance to pitch in is a scaled version of the newcomer's talkativeness.
      const double chance = newcomers.pitchInScale * newcomer.talkativeness * heatGate;
      if (!std::bernoulli_distribution(chance)(rng)) {
        continue;
      }

      double startSeconds;
      bool jumpedIn = false;
      if (std::bernoulli_distribution(newcomers.jumpInProbability)(rng)) {
        // Over the original turn: a collision.
        jumpedIn = true;
        startSeconds = turn.startSeconds +
                       drawFrom(timing.overlapOnsetMilliseconds, rng) / kMillisecondsPerSecond;
      } else {
        // Into the silence, clear of both edges. Silences too short for that are skipped.
        const double silenceMilliseconds =
            (silenceEndSeconds - silenceStartSeconds) * kMillisecondsPerSecond;
        if (silenceMilliseconds < newcomers.minSilenceMilliseconds) {
          continue;
        }
        const double margin = newcomers.edgeMarginMilliseconds / kMillisecondsPerSecond;
        startSeconds = drawFrom({silenceStartSeconds + margin, silenceEndSeconds - margin}, rng);
      }

      if (startSeconds >= durationSeconds ||
          startSeconds - lastPitchInSeconds[n] < newcomers.minSpacingSeconds) {
        continue;
      }
      lastPitchInSeconds[n] = startSeconds;
      conversation.interjections.push_back({newcomer.slot, startSeconds, jumpedIn});
      conversation.chirps.push_back({newcomer.slot, startSeconds});
    }
  }
  return conversation;
}
