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

double drawFrom(const Range& range, std::mt19937& rng) {
  return std::uniform_real_distribution<double>(range.min, range.max)(rng);
}

/** Position `amount` (0..1) of the way from `from` to `to`. */
double blend(double from, double to, double amount) {
  return from + (to - from) * amount;
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

}  // namespace

ConversationGenerator::ConversationGenerator(const ConversationConfig& config,
                                             const DetectorProfile* profiles)
    : config_(config), profiles_(profiles) {}

Conversation ConversationGenerator::generate() const {
  const TimingConfig& timing = config_.timing;
  const TemperamentConfig& temperament = config_.temperament;
  std::mt19937 rng(config_.randomSeed);

  // One generator, consumed in a fixed order, so a seed fixes the whole conversation.
  HeatCurve heat = HeatCurve::draw(config_.heat, config_.durationSeconds, rng);

  std::vector<Participant> cast;
  for (unsigned slot : {config_.firstCastSlot, config_.secondCastSlot}) {
    cast.push_back({slot, drawFrom(config_.temperament.talkativeness, rng),
                    drawFrom(config_.temperament.wordiness, rng)});
  }

  // Talkativeness is a relative weight: the next speaker is drawn in proportion to it. The
  // current speaker stays in the draw, so a chatty slot can speak several turns in a row.
  std::vector<double> weights;
  for (const Participant& participant : cast) {
    weights.push_back(participant.talkativeness);
  }
  std::discrete_distribution<std::size_t> pickSpeaker(weights.begin(), weights.end());

  Conversation conversation{cast, heat, {}, {}};

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

  while (true) {
    const Participant& participant = cast[speaker];
    const double chirpLengthSeconds =
        profiles_[participant.slot - 1].chirpLengthMilliseconds / kMillisecondsPerSecond;

    const double startSeconds = std::max(
        nextStartSeconds,
        lastChirpEndSeconds[speaker] + timing.minSameSlotGapMilliseconds / kMillisecondsPerSecond);
    if (startSeconds >= config_.durationSeconds) {
      break;
    }
    const double heatNow = heat.heatAt(startSeconds);

    // A turn is one chirp, or rarely two: someone adding on to what they just said. The chance
    // grows with heat and with the slot's wordiness.
    const double doubleChance =
        temperament.maxDoubleChirpProbability * participant.wordiness *
        blend(temperament.coldDoubleChirpFraction, 1.0, heatNow);
    const unsigned chirpCount = std::bernoulli_distribution(doubleChance)(rng) ? 2u : 1u;
    for (unsigned i = 0; i < chirpCount; ++i) {
      const double chirpStartSeconds =
          startSeconds + i * timing.doubleChirpSpacingMilliseconds / kMillisecondsPerSecond;
      if (chirpStartSeconds >= config_.durationSeconds) {
        break;
      }
      conversation.chirps.push_back({participant.slot, chirpStartSeconds});
      lastChirpEndSeconds[speaker] = chirpStartSeconds + chirpLengthSeconds;
    }

    // The unheard utterance this turn stands in for: skewed, and a little shorter when hot.
    double utteranceMilliseconds = drawSkewed(
        timing.utteranceMedianMilliseconds * blend(1.0, timing.hotUtteranceScale, heatNow),
        timing.utteranceSpread, rng);
    if (chirpCount == 2) {
      utteranceMilliseconds += timing.doubleChirpSpacingMilliseconds;
    }

    // Who speaks next, and how they come in.
    const std::size_t nextSpeaker = pickSpeaker(rng);
    const bool sameSpeakerNext = nextSpeaker == speaker;

    // Lapses belong to the wind-down: only after the peak, and more likely the cooler it is.
    const double windDown =
        smoothstep((startSeconds - heat.peakTimeSeconds()) /
                   std::max(config_.durationSeconds - heat.peakTimeSeconds(), 1e-9));
    const double lapseChance = timing.lapseMaxProbability * (1.0 - heatNow) * windDown;

    double gapMilliseconds;
    bool overlapNext = false;
    bool lapseNext = false;
    double intervalMilliseconds;

    const double overlapChance =
        blend(timing.coldOverlapProbability, timing.hotOverlapProbability, heatNow);
    if (!sameSpeakerNext && std::bernoulli_distribution(overlapChance)(rng)) {
      // Jumping in: the next speaker starts almost on top of this turn's start.
      overlapNext = true;
      intervalMilliseconds = drawFrom(timing.overlapOnsetMilliseconds, rng);
      gapMilliseconds = intervalMilliseconds - utteranceMilliseconds;
    } else if (std::bernoulli_distribution(lapseChance)(rng)) {
      lapseNext = true;
      gapMilliseconds = drawFrom(timing.lapseSeconds, rng) * kMillisecondsPerSecond;
      intervalMilliseconds = utteranceMilliseconds + gapMilliseconds;
    } else {
      // An ordinary pause; a slot keeping the floor takes a little longer, as if thinking.
      gapMilliseconds = drawSkewed(timing.gapMedianMilliseconds, timing.gapSpread, rng);
      if (sameSpeakerNext) {
        gapMilliseconds += timing.sameSpeakerExtraPauseMilliseconds;
      }
      intervalMilliseconds = utteranceMilliseconds + gapMilliseconds;
    }

    // The floor only stops a jump-in from collapsing onto the same instant.
    intervalMilliseconds = std::max(intervalMilliseconds, timing.minTurnSpacingMilliseconds);
    const double intervalSeconds = intervalMilliseconds / kMillisecondsPerSecond;

    conversation.turns.push_back({participant.slot, startSeconds, chirpCount, heatNow,
                                  intervalSeconds, gapBeforeSeconds, followsSameSpeaker,
                                  overlapsPrevious, afterLapse});

    // Set up the next turn.
    gapBeforeSeconds = gapMilliseconds / kMillisecondsPerSecond;
    followsSameSpeaker = sameSpeakerNext;
    overlapsPrevious = overlapNext;
    afterLapse = lapseNext;
    speaker = nextSpeaker;
    nextStartSeconds = startSeconds + intervalSeconds;
  }

  // The last turn has no successor inside the conversation.
  if (!conversation.turns.empty()) {
    conversation.turns.back().intervalToNextSeconds = 0.0;
  }
  return conversation;
}
