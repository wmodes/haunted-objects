/**
 * @file ConversationGenerator.cpp
 * @brief Implementation of ConversationGenerator. See ConversationGenerator.h.
 */
#include "ConversationGenerator.h"

#include <algorithm>
#include <limits>
#include <random>

namespace {

double drawFrom(const Range& range, std::mt19937& rng) {
  return std::uniform_real_distribution<double>(range.min, range.max)(rng);
}

/** Position `amount` (0..1) of the way from `cold` to `hot`. */
double blend(double cold, double hot, double amount) {
  return cold + (hot - cold) * amount;
}

/** The range `amount` (0..1) of the way from `cold` to `hot`, end by end. */
Range blend(const Range& cold, const Range& hot, double amount) {
  return {blend(cold.min, hot.min, amount), blend(cold.max, hot.max, amount)};
}

constexpr double kMillisecondsPerSecond = 1000.0;

}  // namespace

ConversationGenerator::ConversationGenerator(const ConversationConfig& config,
                                             const DetectorProfile* profiles)
    : config_(config), profiles_(profiles) {}

Conversation ConversationGenerator::generate() const {
  const TimingConfig& timing = config_.timing;
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

  double turnStartSeconds = 0.0;
  while (turnStartSeconds < config_.durationSeconds) {
    const std::size_t speaker = pickSpeaker(rng);
    const Participant& participant = cast[speaker];
    const double chirpLengthSeconds =
        profiles_[participant.slot - 1].chirpLengthMilliseconds / kMillisecondsPerSecond;

    const double startSeconds = std::max(
        turnStartSeconds,
        lastChirpEndSeconds[speaker] + timing.minSameSlotGapMilliseconds / kMillisecondsPerSecond);
    if (startSeconds >= config_.durationSeconds) {
      break;
    }

    // A turn is one chirp, or two when this slot is feeling wordy.
    const unsigned chirpCount =
        std::bernoulli_distribution(participant.wordiness)(rng) ? 2u : 1u;
    for (unsigned i = 0; i < chirpCount; ++i) {
      const double chirpStartSeconds =
          startSeconds + i * timing.doubleChirpSpacingMilliseconds / kMillisecondsPerSecond;
      if (chirpStartSeconds >= config_.durationSeconds) {
        break;
      }
      conversation.chirps.push_back({participant.slot, chirpStartSeconds});
      lastChirpEndSeconds[speaker] = chirpStartSeconds + chirpLengthSeconds;
    }

    // Pace: heat slides the turn length and the gap from their cold to their hot settings.
    const double heatNow = heat.heatAt(startSeconds);
    const Range turnLengthMilliseconds = blend(timing.coldTurnLengthMilliseconds,
                                               timing.hotTurnLengthMilliseconds, heatNow);
    double turnLength = drawFrom(turnLengthMilliseconds, rng);
    if (chirpCount == 2) {
      turnLength += timing.doubleChirpSpacingMilliseconds;
    }

    double gapMilliseconds;
    if (std::bernoulli_distribution(timing.longPauseProbability)(rng)) {
      gapMilliseconds = drawFrom(timing.longPauseSeconds, rng) * kMillisecondsPerSecond;
    } else {
      const double gapMode =
          blend(timing.coldGapModeMilliseconds, timing.hotGapModeMilliseconds, heatNow);
      gapMilliseconds = std::normal_distribution<double>(gapMode, timing.gapSpreadMilliseconds)(rng);
    }

    // A negative interval is talking over; the floor only stops it from collapsing to nothing.
    const double intervalMilliseconds =
        std::max(turnLength + gapMilliseconds, timing.minTurnSpacingMilliseconds);
    const double intervalSeconds = intervalMilliseconds / kMillisecondsPerSecond;

    conversation.turns.push_back(
        {participant.slot, startSeconds, chirpCount, heatNow, intervalSeconds});
    turnStartSeconds = startSeconds + intervalSeconds;
  }

  // The last turn has no successor inside the conversation.
  if (!conversation.turns.empty()) {
    conversation.turns.back().intervalToNextSeconds = 0.0;
  }
  return conversation;
}
