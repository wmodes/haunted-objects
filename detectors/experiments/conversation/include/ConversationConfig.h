/**
 * @file ConversationConfig.h
 * @brief Settings that belong only to the conversation experiment.
 *
 * Anything generally useful to the piece lives in the piece's own config; anything only this
 * experiment needs lives here, so deleting the experiment leaves no stray settings behind. Most
 * values below are starting points to tune by ear, not sourced figures; the one sourced anchor is
 * that human turn gaps are most often about 200 ms (see the Design Document).
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
 * the ranges below, so no two conversations have the same arc.
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
 * Timing between turns. The time from one turn's start to the next turn's start is a turn length
 * plus a gap. Heat slides both from their "cold" settings to their "hot" settings. A gap or
 * interval that comes out below zero means the next turn starts before the last one "finishes":
 * talking over.
 */
struct TimingConfig {
  /** Most likely gap between turns when the conversation is cold, in milliseconds. */
  double coldGapModeMilliseconds = 700.0;

  /** Most likely gap between turns when the conversation is hot, in milliseconds. */
  double hotGapModeMilliseconds = 30.0;

  /**
   * Spread of the gap around its most likely value, in milliseconds (the standard deviation).
   * Raising it makes the rhythm looser and more human, and makes talking over more frequent when
   * hot.
   */
  double gapSpreadMilliseconds = 120.0;

  /** Turn length range when cold, in milliseconds. A chirp stands in for a spoken turn. */
  Range coldTurnLengthMilliseconds{500.0, 1500.0};

  /** Turn length range when hot, in milliseconds. Hot exchanges are clipped and rapid. */
  Range hotTurnLengthMilliseconds{80.0, 300.0};

  /** Chance that a turn is followed by a long pause instead of a normal gap, 0 to 1. */
  double longPauseProbability = 0.03;

  /** Length range of a long pause, in seconds. */
  Range longPauseSeconds{1.5, 4.0};

  /** Time between the two chirps of a two-chirp turn, in milliseconds. */
  double doubleChirpSpacingMilliseconds = 180.0;

  /**
   * The soonest a turn may start after the previous turn started, in milliseconds. Keeps
   * talking over from collapsing into a single chirp. Raising it means fewer near-collisions.
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

  /** Chance that a slot's turn is two chirps instead of one, 0 to 1. */
  Range wordiness{0.0, 0.5};
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

static_assert(kDefaults.timing.gapSpreadMilliseconds >= 0.0,
              "gapSpreadMilliseconds must not be negative");
static_assert(isOrdered(kDefaults.timing.coldTurnLengthMilliseconds) &&
                  kDefaults.timing.coldTurnLengthMilliseconds.min >= 0.0,
              "coldTurnLengthMilliseconds must be an ordered, non-negative range");
static_assert(isOrdered(kDefaults.timing.hotTurnLengthMilliseconds) &&
                  kDefaults.timing.hotTurnLengthMilliseconds.min >= 0.0,
              "hotTurnLengthMilliseconds must be an ordered, non-negative range");
static_assert(isProbability(kDefaults.timing.longPauseProbability),
              "longPauseProbability must be within 0..1");
static_assert(isOrdered(kDefaults.timing.longPauseSeconds), "longPauseSeconds must be ordered");
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

static_assert(kDefaults.audio.sampleRateHertz > 0, "sampleRateHertz must be positive");
static_assert(kDefaults.audio.amplitude > 0.0 && kDefaults.audio.amplitude <= 1.0,
              "amplitude must be in (0, 1]");
static_assert(kDefaults.audio.fadeMilliseconds >= 0.0, "fadeMilliseconds must not be negative");

}  // namespace
