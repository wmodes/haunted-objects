/**
 * @file ConversationGenerator.h
 * @brief Generates one conversation between slots as a score of timed chirps.
 *
 * A pure function of the settings and a seed: the same seed gives the same conversation, and the
 * generator touches no audio or hardware. It produces timing only. Each slot's voice (chirp
 * length and pitch) is applied later, from the detector profile table, by whatever plays the
 * score.
 *
 * Two original speakers hold the conversation; up to three newcomers (other slots) may pitch in,
 * as people at nearby tables might, without joining the turn-taking. A chirp is a turn's whole speech,
 * so the only timing is the silence between chirps: skewed, bounded, and sized by how hard the
 * previous turn was to answer. Heat drives jump-ins (none in the cool opening) and shortens
 * thinking pauses a little, same-speaker continuations pause longer, and lapses (long silences)
 * come only in the wind-down.
 */
#pragma once

#include <cstddef>
#include <vector>

#include "ConversationConfig.h"
#include "DetectorProfiles.h"
#include "HeatCurve.h"

/** One chirp in the score. */
struct ChirpEvent {
  unsigned slot;
  double startSeconds;
};

/** One slot's contribution: when it started, how many chirps, and how it was reached. */
struct ConversationTurn {
  unsigned slot;
  double startSeconds;
  unsigned chirpCount;
  /** Heat at the moment the turn started. */
  double heat;
  /** How hard this turn was to answer (0 trivial, 1 hard); it sets the pause before the reply. */
  double difficulty;
  /** How hard the turn this one answers was; it sets this turn's pause and inner chirp spacing. */
  double demand;
  /** Time from this turn's start to the next turn's start; 0 for the final turn. */
  double intervalToNextSeconds;

  /**
   * Silence between the previous turn's last chirp and this turn's first, in seconds. For a
   * jump-in it is the short delay after the previous turn's start. 0 for the first turn.
   */
  double gapBeforeSeconds;
  /** True if the same slot also took the previous turn (holding the floor). */
  bool followsSameSpeaker;
  /** True if this turn jumped in on top of the previous one. */
  bool overlapsPrevious;
  /** True if this turn came after a lapse, a long silence. */
  bool afterLapse;
  /** Time from this turn's start to its last chirp, in seconds (0 for a one-chirp turn). */
  double lastChirpOffsetSeconds;
};

/** A slot's traits for one conversation. Drawn fresh each time, unlike the fixed voice. */
struct Participant {
  unsigned slot;
  double talkativeness;
  double wordiness;
  /** True for the two speakers who hold the conversation; false for a newcomer pitching in. */
  bool isOriginal;
};

/**
 * One chirp a newcomer pitched in. Newcomers never take a turn in the original exchange; they
 * chirp into a silence or, occasionally, on top of an original turn.
 */
struct Interjection {
  unsigned slot;
  double startSeconds;
  /** True if it landed on top of an original turn (a collision), false if in a silence. */
  bool jumpedIn;
};

struct Conversation {
  /** Length of this conversation, drawn before anything else and independent of the timing. */
  double durationSeconds;
  /** The two original speakers first, then any newcomers present in this conversation. */
  std::vector<Participant> cast;
  HeatCurve heat;
  /** The original speakers' turns only; newcomers' chirps are in `interjections`. */
  std::vector<ConversationTurn> turns;
  std::vector<Interjection> interjections;
  /** Every chirp of every turn and every interjection, in the order they were placed. */
  std::vector<ChirpEvent> chirps;
};

class ConversationGenerator {
 public:
  /**
   * @param config Settings for the conversation.
   * @param profiles The detector profile table, indexed so that profiles[slot - 1] is that slot.
   *                 The settings' compile-time checks guarantee the cast slots are in the table.
   */
  ConversationGenerator(const ConversationConfig& config, const DetectorProfile* profiles);

  /** Generates the conversation. Deterministic for a given seed. */
  Conversation generate() const;

 private:
  ConversationConfig config_;
  const DetectorProfile* profiles_;
};
