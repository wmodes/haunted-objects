/**
 * @file ConversationConfig.h
 * @brief Settings that belong only to the conversation experiment.
 *
 * Anything generally useful to the piece lives in the piece's own config; anything only this
 * experiment needs lives here, so deleting the experiment leaves no stray settings behind. Most
 * values below are starting points to tune by ear. The timing anchors come from the turn-taking
 * literature, read through search summaries (see the Design Document): median spoken utterance
 * about 0.9 s, median gap between speakers roughly 100-200 ms with a long tail, and same-speaker
 * pauses about 140 ms longer than speaker-change gaps.
 */
#pragma once

#include <cstddef>

#include "DetectorProfiles.h"

/** A closed range of values; something drawn "from a range" is drawn uniformly between the ends. */
struct Range {
  double min;
  double max;
};

/** The shape of the wave rendered for each chirp. */
enum class Waveform {
  Sine,      ///< Smooth tone; closest to what a horn's resonant cavity radiates.
  Square,    ///< What a push-pull H-bridge drives onto the horn electrically.
  Sawtooth,  ///< A rising ramp; buzzier, with both odd and even harmonics.
};

/**
 * Heat follows a bell curve over the conversation. Each conversation draws its own curve from
 * the ranges below, so no two conversations have the same arc. Heat drives overlap and (in later
 * steps) recruitment, and shortens utterances a little; it does not slow the opening, because
 * conversations open briskly.
 */
struct HeatConfig {
  /** How hot the conversation gets at its peak, 0 to 1. Higher is a more heated exchange. */
  Range peakHeat{0.7, 1.0};

  /** Where the peak falls, as a fraction of the conversation's length (0 = start, 1 = end). */
  Range peakTimeFraction{0.35, 0.65};

  /**
   * Width of the bell, as a fraction of the conversation's length (the standard deviation of the
   * curve). Smaller is a short flare-up; larger is a long simmer.
   */
  Range widthFraction{0.12, 0.28};

  /** Heat that the conversation never drops below, 0 to 1. Raising it keeps the edges warmer. */
  double baselineHeat = 0.10;

  /** Size of the slow random wobble added to the curve, in heat units. 0 gives a smooth bell. */
  double wobbleAmount = 0.06;

  /** Period of the slowest wobble, in seconds. Longer is a lazier drift. */
  double wobblePeriodSeconds = 12.0;

  /**
   * How many times faster the second, quicker wobble runs than the slow one. Two wobbles at
   * unrelated speeds never repeat exactly, so the heat drifts instead of cycling. Must be above 1.
   */
  double fastWobbleRatio = 2.7;
};

/**
 * Timing between turns. Each chirp marks an unheard utterance, so the time from one turn's start
 * to the next turn's start is the implied speaking time plus a gap. Utterances and gaps are
 * skewed (log-normal): mostly short, with a long tail of occasional real pauses.
 */
struct TimingConfig {
  /** Median length of the speech a chirp stands in for, in milliseconds. Higher is slower. */
  double utteranceMedianMilliseconds = 900.0;

  /**
   * Spread of utterance lengths (the log-normal sigma). 0 makes every utterance the median;
   * around 0.8 puts a quarter under about 0.5 s and a quarter over about 1.5 s.
   */
  double utteranceSpread = 0.8;

  /**
   * Factor applied to the utterance median at full heat; 1 means heat does not shorten
   * utterances, and below 1 makes a hot exchange more clipped. Interpolated by heat.
   */
  double hotUtteranceScale = 0.8;

  /** Median silence between one speaker finishing and another starting, in milliseconds. */
  double gapMedianMilliseconds = 150.0;

  /**
   * Spread of the gap (the log-normal sigma). Larger gives a longer tail of slow replies;
   * around 0.8 puts the 95th percentile near four times the median.
   */
  double gapSpread = 0.8;

  /**
   * Extra pause when the same slot keeps the floor instead of passing it, in milliseconds. A slot
   * that continues sounds like it is still thinking. Raising it makes that hesitation clearer.
   */
  double sameSpeakerExtraPauseMilliseconds = 140.0;

  /** Chance, when cold, that the next speaker jumps in on top of the current one, 0 to 1. */
  double coldOverlapProbability = 0.04;

  /** Chance, when hot, that the next speaker jumps in on top of the current one, 0 to 1. */
  double hotOverlapProbability = 0.35;

  /**
   * When a speaker jumps in, how soon after the other's turn started it lands, in milliseconds.
   * Chirps are about 10 ms long, so small values make near-simultaneous chirps.
   */
  Range overlapOnsetMilliseconds{10.0, 150.0};

  /**
   * Chance of a lapse (a long silence) after a turn, at its greatest: when heat is zero and the
   * conversation is at its very end, 0 to 1. Lapses only become likely after the peak, as heat
   * falls, so the opening stays brisk and the ending trails off.
   */
  double lapseMaxProbability = 0.25;

  /** Length range of a lapse, in seconds. */
  Range lapseSeconds{1.5, 5.0};

  /** Time between the two chirps of a two-chirp turn, in milliseconds. */
  double doubleChirpSpacingMilliseconds = 180.0;

  /**
   * The soonest a turn may start after the previous turn started, in milliseconds. Keeps
   * talking over from collapsing into a single chirp.
   */
  double minTurnSpacingMilliseconds = 5.0;

  /**
   * The soonest a slot may chirp again after its own previous chirp ended, in milliseconds. A
   * horn cannot start a new chirp on top of its own.
   */
  double minSameSlotGapMilliseconds = 25.0;
};

/** Ranges the per-slot temperament is drawn from, fresh for each conversation. */
struct TemperamentConfig {
  /**
   * How readily a slot is picked to speak next, as a relative weight. A slot drawn near the top of
   * the range talks far more than one near the bottom. Must be above 0.
   */
  Range talkativeness{0.15, 1.0};

  /**
   * How likely a slot is to add a second chirp to its turn, relative to the most wordy (0 to 1).
   * The actual chance also scales with heat; see the two settings below.
   */
  Range wordiness{0.2, 1.0};

  /**
   * Chance that a turn is two chirps, for the wordiest slot at full heat, 0 to 1. Two-chirp
   * turns are rare: someone adding on to what they just said. Raising it makes them commoner.
   */
  double maxDoubleChirpProbability = 0.25;

  /** Share of that chance that remains when the conversation is cold, 0 to 1. */
  double coldDoubleChirpFraction = 0.15;
};

/** How the score is turned into sound. */
struct AudioConfig {
  /** Audio sample rate, in hertz. Raising it improves quality and grows the file. */
  unsigned sampleRateHertz = 44100;

  /**
   * Overall loudness of the mix, from 0 (silent) to 1 (full scale). Raising it makes the chirps
   * louder; overlapping chirps are clipped at 1.
   */
  double amplitude = 0.5;

  /**
   * Length of the fade-in and fade-out on each chirp, in milliseconds. Without a fade the abrupt
   * start and end of a short burst clicks. It is capped at half the chirp's length.
   */
  double fadeMilliseconds = 1.0;

  /**
   * Which wave each chirp is drawn with. Sine is the default here because this experiment plays
   * through computer speakers; the detector is expected to drive its horns with Square.
   */
  Waveform waveform = Waveform::Sine;

  /** Where the WAV file is written, relative to where the program is run. */
  const char* outputFilePath = "conversation.wav";
};

struct ConversationConfig {
  /** First of the two slots in the cast. Slots count from 1, as in the profile table. */
  unsigned firstCastSlot = 1;

  /** Second of the two slots in the cast. */
  unsigned secondCastSlot = 2;

  /** Length of the conversation, in seconds. */
  double durationSeconds = 90.0;

  /**
   * Seed for everything random in the conversation (heat curve, temperaments, speaker order,
   * gaps). The same seed gives the same conversation; change it to hear a different one.
   */
  unsigned randomSeed = 20261112;

  /**
   * How many conversations (seeds counting up from randomSeed) to generate for the summary of
   * statistics. More seeds make the averages steadier.
   */
  unsigned statisticsConversationCount = 300;

  HeatConfig heat;
  TimingConfig timing;
  TemperamentConfig temperament;
  AudioConfig audio;
};

namespace conversation_config_internal {

constexpr bool isOrdered(const Range& range) { return range.min <= range.max; }
constexpr bool isProbability(double value) { return value >= 0.0 && value <= 1.0; }

}  // namespace conversation_config_internal

// Compile-time validation: a bad setting is a build error, not a runtime surprise.
namespace {

constexpr ConversationConfig kDefaults{};
using conversation_config_internal::isOrdered;
using conversation_config_internal::isProbability;

static_assert(kDefaults.firstCastSlot >= 1 && kDefaults.firstCastSlot <= kDetectorProfileCount,
              "firstCastSlot must be a slot in the profile table");
static_assert(kDefaults.secondCastSlot >= 1 && kDefaults.secondCastSlot <= kDetectorProfileCount,
              "secondCastSlot must be a slot in the profile table");
static_assert(kDefaults.firstCastSlot != kDefaults.secondCastSlot,
              "the cast needs two different slots");
static_assert(kDefaults.durationSeconds > 0.0, "durationSeconds must be positive");
static_assert(kDefaults.statisticsConversationCount > 0,
              "statisticsConversationCount must be positive");

static_assert(isOrdered(kDefaults.heat.peakHeat) && isProbability(kDefaults.heat.peakHeat.min) &&
                  isProbability(kDefaults.heat.peakHeat.max),
              "peakHeat must be an ordered range within 0..1");
static_assert(isOrdered(kDefaults.heat.peakTimeFraction) &&
                  isProbability(kDefaults.heat.peakTimeFraction.min) &&
                  isProbability(kDefaults.heat.peakTimeFraction.max),
              "peakTimeFraction must be an ordered range within 0..1");
static_assert(isOrdered(kDefaults.heat.widthFraction) && kDefaults.heat.widthFraction.min > 0.0,
              "widthFraction must be an ordered range above 0");
static_assert(isProbability(kDefaults.heat.baselineHeat), "baselineHeat must be within 0..1");
static_assert(kDefaults.heat.wobbleAmount >= 0.0, "wobbleAmount must not be negative");
static_assert(kDefaults.heat.wobblePeriodSeconds > 0.0, "wobblePeriodSeconds must be positive");
static_assert(kDefaults.heat.fastWobbleRatio > 1.0, "fastWobbleRatio must be above 1");

static_assert(kDefaults.timing.utteranceMedianMilliseconds > 0.0,
              "utteranceMedianMilliseconds must be positive");
static_assert(kDefaults.timing.utteranceSpread >= 0.0, "utteranceSpread must not be negative");
static_assert(kDefaults.timing.hotUtteranceScale > 0.0, "hotUtteranceScale must be positive");
static_assert(kDefaults.timing.gapMedianMilliseconds > 0.0,
              "gapMedianMilliseconds must be positive");
static_assert(kDefaults.timing.gapSpread >= 0.0, "gapSpread must not be negative");
static_assert(kDefaults.timing.sameSpeakerExtraPauseMilliseconds >= 0.0,
              "sameSpeakerExtraPauseMilliseconds must not be negative");
static_assert(isProbability(kDefaults.timing.coldOverlapProbability) &&
                  isProbability(kDefaults.timing.hotOverlapProbability),
              "overlap probabilities must be within 0..1");
static_assert(isOrdered(kDefaults.timing.overlapOnsetMilliseconds) &&
                  kDefaults.timing.overlapOnsetMilliseconds.min >= 0.0,
              "overlapOnsetMilliseconds must be an ordered, non-negative range");
static_assert(isProbability(kDefaults.timing.lapseMaxProbability),
              "lapseMaxProbability must be within 0..1");
static_assert(isOrdered(kDefaults.timing.lapseSeconds) && kDefaults.timing.lapseSeconds.min >= 0.0,
              "lapseSeconds must be an ordered, non-negative range");
static_assert(kDefaults.timing.doubleChirpSpacingMilliseconds > 0.0,
              "doubleChirpSpacingMilliseconds must be positive");
static_assert(kDefaults.timing.minTurnSpacingMilliseconds >= 0.0,
              "minTurnSpacingMilliseconds must not be negative");
static_assert(kDefaults.timing.minSameSlotGapMilliseconds >= 0.0,
              "minSameSlotGapMilliseconds must not be negative");

static_assert(isOrdered(kDefaults.temperament.talkativeness) &&
                  kDefaults.temperament.talkativeness.min > 0.0,
              "talkativeness must be an ordered range above 0");
static_assert(isOrdered(kDefaults.temperament.wordiness) &&
                  isProbability(kDefaults.temperament.wordiness.min) &&
                  isProbability(kDefaults.temperament.wordiness.max),
              "wordiness must be an ordered range within 0..1");
static_assert(isProbability(kDefaults.temperament.maxDoubleChirpProbability),
              "maxDoubleChirpProbability must be within 0..1");
static_assert(isProbability(kDefaults.temperament.coldDoubleChirpFraction),
              "coldDoubleChirpFraction must be within 0..1");

static_assert(kDefaults.audio.sampleRateHertz > 0, "sampleRateHertz must be positive");
static_assert(kDefaults.audio.amplitude > 0.0 && kDefaults.audio.amplitude <= 1.0,
              "amplitude must be in (0, 1]");
static_assert(kDefaults.audio.fadeMilliseconds >= 0.0, "fadeMilliseconds must not be negative");

}  // namespace
