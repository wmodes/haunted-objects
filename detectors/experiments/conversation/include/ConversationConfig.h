/**
 * @file ConversationConfig.h
 * @brief Settings that belong only to the conversation experiment.
 *
 * Anything generally useful to the piece lives in the piece's own config; anything only this
 * experiment needs lives here, so deleting the experiment leaves no stray settings behind. Most
 * values below are starting points to tune by ear. A chirp *is* a turn's speech, so the only
 * timing is the silence between chirps; its size follows how hard the last turn was to answer.
 * The literature anchors (see the Design Document) are that pauses are skewed (mostly short,
 * with a long tail) and that a same-speaker continuation pauses about 140 ms longer.
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
 * steps) recruitment, and shortens thinking pauses a little; it does not slow the opening,
 * because conversations open briskly.
 */
struct HeatConfig {
  /**
   * How hot the conversation gets at its peak, 0 to 1. Higher is a more heated exchange. The
   * bottom is above the heat at which the heated pacing and the newcomers reach full strength
   * (0.8), so every conversation gets properly hot; lowering it allows cooler conversations.
   */
  Range peakHeat{0.85, 1.0};

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
 * Difficulty is how much thought it takes to answer a turn, from 0 (trivial, like "Hi") to 1 (hard,
 * like a probing question). It sets how long the next speaker pauses to think, so conversations
 * lope: quick small talk, a stall at a hard question, then easing back. Difficulty has memory: it
 * drifts from turn to turn, with occasional topic shifts to a new level.
 */
struct DifficultyConfig {
  /** Difficulty of what the first turn answers, 0 to 1. Low means the opening is small talk. */
  double openingDifficulty = 0.1;

  /**
   * The hardest a turn can be early in the conversation, 0 to 1: before the heat peak and while
   * it is still cool, talk stays light and the thinking pauses stay short. The cap rises to 1 as
   * heat builds (over the same heat range as the heated pacing) and is gone after the peak, so
   * the long pauses of the wind-down are untouched. Lower makes the early pauses shorter.
   */
  double coolMaxDifficulty = 0.55;

  /** Chance, per turn, of a topic shift: difficulty jumps to a fresh level, 0 to 1. */
  double topicShiftProbability = 0.3;

  /**
   * How strongly a fresh level is biased toward easy. A new level is a uniform draw raised to
   * this power: 1 is evenly spread, higher makes most topics easy and only a few hard. Above 0.
   */
  double topicShiftSkew = 1.8;

  /**
   * Size of the turn-to-turn drift when there is no topic shift (the standard deviation, in
   * difficulty units). Larger makes difficulty wander faster; 0 holds it until the next shift.
   */
  double driftSpread = 0.12;
};

/**
 * Timing between turns: the silence between one chirp and the next. It is skewed (log-normal),
 * mostly near its median with a long-ish tail, and the median slides with difficulty.
 */
struct TimingConfig {
  /** Median silence before replying to a trivial turn (difficulty 0), in milliseconds. */
  double easyGapMedianMilliseconds = 300.0;

  /** Median silence before replying to a hard turn (difficulty 1), in milliseconds. */
  double hardGapMedianMilliseconds = 3600.0;

  /**
   * Spread of the silence around its median (the log-normal sigma). Difficulty explains most of
   * the variety, so this is modest. Larger gives more surprise.
   */
  double gapSpread = 0.3;

  /**
   * Heat below which the heated stretch's pacing has no effect at all, 0 to 1. Keeps the cool
   * opening exactly as it is; the sparser pacing below only ramps in as heat builds.
   */
  double heatedStartHeat = 0.35;

  /**
   * Heat at which the heated stretch's pacing reaches full strength, 0 to 1 (above
   * heatedStartHeat). The settings below ramp smoothly between the two.
   */
  double heatedFullHeat = 0.8;

  /**
   * Factor applied to the two original speakers' pauses at full strength of the heated pacing.
   * Above 1 makes the heated stretch sparser, which leaves room for others to pitch in; 1 means no
   * change; below 1 makes heated people answer faster.
   */
  double hotThinkingScale = 2.5;

  /**
   * Extra silence when the same slot keeps the floor instead of passing it, in milliseconds. A
   * slot that continues sounds like it is still thinking. Raising it makes that clearer.
   */
  double sameSpeakerExtraPauseMilliseconds = 140.0;

  /**
   * The shortest silence between ordinary turns when the conversation is cool, in milliseconds. A
   * hard floor: no ordinary pause is quicker. Jump-ins are exempt, because they are meant to
   * crowd. The floor slides to the hot value below as heat rises.
   */
  double minOrdinaryGapMilliseconds = 200.0;

  /**
   * The shortest silence between ordinary turns at full strength of the heated pacing, in
   * milliseconds. Higher than the cool floor, so the two original speakers never rattle off quick
   * replies in the heated stretch.
   */
  double hotMinOrdinaryGapMilliseconds = 600.0;

  /**
   * The longest silence between ordinary turns, in milliseconds. A hard ceiling: no ordinary
   * pause is longer. Lapses are exempt, because they are meant to be long.
   */
  double maxOrdinaryGapMilliseconds = 4800.0;

  /**
   * The longest silence between ordinary turns at full strength of the heated pacing, in
   * milliseconds. The ceiling slides between the cool value above and this one; it is higher
   * because the heated stretch's pauses are stretched, and a lower ceiling would crush them
   * against it.
   */
  double hotMaxOrdinaryGapMilliseconds = 9000.0;

  /**
   * Heat below which nobody jumps in, 0 to 1. Keeps the cool opening orderly: talking over only
   * appears as heat builds.
   */
  double overlapStartHeat = 0.3;

  /**
   * Heat at which the chance of a jump-in reaches its full value, 0 to 1 (above overlapStartHeat).
   * The chance ramps smoothly between the two.
   */
  double overlapFullHeat = 0.8;

  /**
   * Chance that the other original speaker jumps in on top of the current one, at full heat, 0 to
   * 1. Low, because the heated stretch is sparse; newcomers' jump-ins are set separately.
   */
  double hotOverlapProbability = 0.10;

  /**
   * When a speaker jumps in, how soon after the other's turn started it lands, in milliseconds.
   * Chirps are about 10 ms long, so two are heard colliding only if they start within a few tens
   * of milliseconds of each other. Larger values just sound quick, not like talking over.
   */
  Range overlapOnsetMilliseconds{0.0, 35.0};

  /**
   * Chance of a lapse (a long silence) after a turn, at its greatest: when heat is zero and the
   * conversation is at its very end, 0 to 1. Lapses only become likely after the peak, as heat
   * falls, so the opening stays brisk and the ending trails off.
   */
  double lapseMaxProbability = 0.12;

  /** Length range of a lapse, in seconds. */
  Range lapseSeconds{1.5, 5.0};

  /**
   * Silence between the two chirps of a two-chirp turn when answering a trivial turn, in
   * milliseconds ("Well." "Yeah.").
   */
  double doubleChirpSpacingEasyMilliseconds = 182.0;

  /**
   * Silence between the two chirps of a two-chirp turn when answering a hard turn, in
   * milliseconds ("Well... part time."). Interpolated by difficulty.
   */
  double doubleChirpSpacingHardMilliseconds = 1154.0;

  /** Spread of that spacing (the log-normal sigma). 0 makes it exactly the interpolated value. */
  double doubleChirpSpacingSpread = 0.225;

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

/**
 * Newcomers are other slots that pitch in on a conversation they are not part of, like people at
 * nearby tables. They are not equal interlocutors: they never take a turn in the original
 * speakers' exchange, they only chirp into the silences (or occasionally over a turn), and their
 * chirps are single. Few pitch in: a conversation has up to three, and each one's chance of
 * pitching in is a scaled-down version of its talkativeness.
 */
struct NewcomerConfig {
  /** First slot a newcomer may be drawn from. Must not be one of the two original speakers. */
  unsigned poolFirstSlot = 3;

  /** Last slot a newcomer may be drawn from. */
  unsigned poolLastSlot = 6;

  /**
   * Relative chance that a conversation has 0, 1, 2, or 3 newcomers present (the weights need not
   * sum to 1). Few is likelier than many; more than three never pitch in.
   */
  double countWeights[4] = {0.10, 0.40, 0.35, 0.15};

  /**
   * How a newcomer's talkativeness is scaled into its chance of pitching in after an original
   * turn: chance = pitchInScale x talkativeness x the heat gate below. Raising it makes newcomers
   * pitch in more often; small values keep them rare.
   */
  double pitchInScale = 0.35;

  /** Heat below which no newcomer pitches in, 0 to 1. Nearby tables join as it gets vivid. */
  double startHeat = 0.35;

  /** Heat at which the heat gate reaches full strength, 0 to 1 (above startHeat). */
  double fullHeat = 0.8;

  /**
   * Chance that a pitch-in lands on top of an original turn (a collision) instead of in the silence
   * after it, 0 to 1.
   */
  double jumpInProbability = 0.25;

  /** The least time between one newcomer's pitch-ins, in seconds. Stops a newcomer machine-gunning. */
  double minSpacingSeconds = 2.5;

  /** The shortest silence a newcomer will pitch into, in milliseconds. Shorter ones are skipped. */
  double minSilenceMilliseconds = 600.0;

  /** How far from either edge of a silence a newcomer's chirp lands, in milliseconds. */
  double edgeMarginMilliseconds = 250.0;
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

  /**
   * Length range of a conversation, in seconds. The length is drawn once, first, before anything
   * else, and it is independent of the timing between turns: a longer conversation has more
   * turns at the same pace, not slower ones. The heat curve and the wind-down scale to it. Set
   * both ends equal to fix the length, for example to compare two models for the same time.
   */
  Range durationSeconds{60.0, 240.0};

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
  DifficultyConfig difficulty;
  TimingConfig timing;
  TemperamentConfig temperament;
  NewcomerConfig newcomers;
  AudioConfig audio;
};

namespace conversation_config_internal {

constexpr bool isOrdered(const Range& range) { return range.min <= range.max; }
constexpr bool isProbability(double value) { return value >= 0.0 && value <= 1.0; }

/** True if no weight is negative and at least one is positive. */
template <std::size_t N>
constexpr bool weightsAreUsable(const double (&weights)[N]) {
  bool anyPositive = false;
  for (double weight : weights) {
    if (weight < 0.0) {
      return false;
    }
    anyPositive = anyPositive || weight > 0.0;
  }
  return anyPositive;
}

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
static_assert(isOrdered(kDefaults.durationSeconds) && kDefaults.durationSeconds.min > 0.0,
              "durationSeconds must be an ordered range above 0");
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

static_assert(isProbability(kDefaults.difficulty.openingDifficulty),
              "openingDifficulty must be within 0..1");
static_assert(isProbability(kDefaults.difficulty.topicShiftProbability),
              "topicShiftProbability must be within 0..1");
static_assert(isProbability(kDefaults.difficulty.coolMaxDifficulty) &&
                  kDefaults.difficulty.coolMaxDifficulty > 0.0,
              "coolMaxDifficulty must be within (0, 1]");
static_assert(kDefaults.difficulty.topicShiftSkew > 0.0, "topicShiftSkew must be above 0");
static_assert(kDefaults.difficulty.driftSpread >= 0.0, "driftSpread must not be negative");

static_assert(kDefaults.timing.easyGapMedianMilliseconds > 0.0,
              "easyGapMedianMilliseconds must be positive");
static_assert(kDefaults.timing.hardGapMedianMilliseconds > 0.0,
              "hardGapMedianMilliseconds must be positive");
static_assert(kDefaults.timing.gapSpread >= 0.0, "gapSpread must not be negative");
static_assert(kDefaults.timing.hotThinkingScale > 0.0, "hotThinkingScale must be positive");
static_assert(kDefaults.timing.sameSpeakerExtraPauseMilliseconds >= 0.0,
              "sameSpeakerExtraPauseMilliseconds must not be negative");
static_assert(kDefaults.timing.minOrdinaryGapMilliseconds > 0.0 &&
                  kDefaults.timing.minOrdinaryGapMilliseconds <
                      kDefaults.timing.maxOrdinaryGapMilliseconds,
              "the ordinary gap bounds must be positive and ordered");
static_assert(isProbability(kDefaults.timing.overlapStartHeat) &&
                  isProbability(kDefaults.timing.overlapFullHeat) &&
                  kDefaults.timing.overlapStartHeat < kDefaults.timing.overlapFullHeat,
              "overlapStartHeat must be below overlapFullHeat, both within 0..1");
static_assert(isProbability(kDefaults.timing.heatedStartHeat) &&
                  isProbability(kDefaults.timing.heatedFullHeat) &&
                  kDefaults.timing.heatedStartHeat < kDefaults.timing.heatedFullHeat,
              "heatedStartHeat must be below heatedFullHeat, both within 0..1");
static_assert(kDefaults.timing.hotMinOrdinaryGapMilliseconds >= 0.0 &&
                  kDefaults.timing.hotMinOrdinaryGapMilliseconds <
                      kDefaults.timing.hotMaxOrdinaryGapMilliseconds,
              "the hot gap bounds must be non-negative and ordered");
static_assert(isProbability(kDefaults.timing.hotOverlapProbability),
              "hotOverlapProbability must be within 0..1");
static_assert(isOrdered(kDefaults.timing.overlapOnsetMilliseconds) &&
                  kDefaults.timing.overlapOnsetMilliseconds.min >= 0.0,
              "overlapOnsetMilliseconds must be an ordered, non-negative range");
static_assert(isProbability(kDefaults.timing.lapseMaxProbability),
              "lapseMaxProbability must be within 0..1");
static_assert(isOrdered(kDefaults.timing.lapseSeconds) && kDefaults.timing.lapseSeconds.min >= 0.0,
              "lapseSeconds must be an ordered, non-negative range");
static_assert(kDefaults.timing.doubleChirpSpacingEasyMilliseconds > 0.0 &&
                  kDefaults.timing.doubleChirpSpacingHardMilliseconds > 0.0,
              "double-chirp spacings must be positive");
static_assert(kDefaults.timing.doubleChirpSpacingSpread >= 0.0,
              "doubleChirpSpacingSpread must not be negative");
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

static_assert(kDefaults.newcomers.poolFirstSlot >= 1 &&
                  kDefaults.newcomers.poolFirstSlot <= kDefaults.newcomers.poolLastSlot &&
                  kDefaults.newcomers.poolLastSlot <= kDetectorProfileCount,
              "the newcomer pool must be an ordered range of slots in the profile table");
static_assert(!(kDefaults.firstCastSlot >= kDefaults.newcomers.poolFirstSlot &&
                kDefaults.firstCastSlot <= kDefaults.newcomers.poolLastSlot) &&
                  !(kDefaults.secondCastSlot >= kDefaults.newcomers.poolFirstSlot &&
                    kDefaults.secondCastSlot <= kDefaults.newcomers.poolLastSlot),
              "the two original speakers must not be in the newcomer pool");
static_assert(conversation_config_internal::weightsAreUsable(kDefaults.newcomers.countWeights),
              "newcomer countWeights must be non-negative with at least one positive");
static_assert(kDefaults.newcomers.pitchInScale >= 0.0 && kDefaults.newcomers.pitchInScale <= 1.0,
              "pitchInScale must be within 0..1");
static_assert(isProbability(kDefaults.newcomers.startHeat) &&
                  isProbability(kDefaults.newcomers.fullHeat) &&
                  kDefaults.newcomers.startHeat < kDefaults.newcomers.fullHeat,
              "newcomer startHeat must be below fullHeat, both within 0..1");
static_assert(isProbability(kDefaults.newcomers.jumpInProbability),
              "newcomer jumpInProbability must be within 0..1");
static_assert(kDefaults.newcomers.minSpacingSeconds >= 0.0,
              "newcomer minSpacingSeconds must not be negative");
static_assert(kDefaults.newcomers.edgeMarginMilliseconds >= 0.0 &&
                  kDefaults.newcomers.minSilenceMilliseconds >
                      2.0 * kDefaults.newcomers.edgeMarginMilliseconds,
              "a newcomer's minimum silence must be longer than both edge margins");

static_assert(kDefaults.audio.sampleRateHertz > 0, "sampleRateHertz must be positive");
static_assert(kDefaults.audio.amplitude > 0.0 && kDefaults.audio.amplitude <= 1.0,
              "amplitude must be in (0, 1]");
static_assert(kDefaults.audio.fadeMilliseconds >= 0.0, "fadeMilliseconds must not be negative");

}  // namespace
