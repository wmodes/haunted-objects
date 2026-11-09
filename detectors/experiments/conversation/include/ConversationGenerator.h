/**
 * @file ConversationGenerator.h
 * @brief Generates one conversation between slots as a score of timed chirps.
 *
 * A pure function of the settings and a seed: the same seed gives the same conversation, and the
 * generator touches no audio or hardware. It produces timing only. Each slot's voice (chirp
 * length and pitch) is applied later, from the detector profile table, by whatever plays the
 * score.
 *
 * Step 1 of the conversation suite: a fixed cast of two slots. Each chirp marks an unheard
 * utterance; the spacing between turns is that implied speech plus a skewed gap. Heat drives
 * overlap (and shortens utterances a little), same-speaker continuations pause longer, and
 * lapses (long silences) come only in the wind-down.
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
  /** Time from this turn's start to the next turn's start; 0 for the final turn. */
  double intervalToNextSeconds;

  /**
   * Silence between the previous turn's implied end and this turn's start, in seconds. Negative
   * means this turn started before the previous one "finished": talking over. 0 for the first.
   */
  double gapBeforeSeconds;
  /** True if the same slot also took the previous turn (holding the floor). */
  bool followsSameSpeaker;
  /** True if this turn jumped in on top of the previous one. */
  bool overlapsPrevious;
  /** True if this turn came after a lapse, a long silence. */
  bool afterLapse;
};

/** A slot's traits for one conversation. Drawn fresh each time, unlike the fixed voice. */
struct Participant {
  unsigned slot;
  double talkativeness;
  double wordiness;
};

struct Conversation {
  std::vector<Participant> cast;
  HeatCurve heat;
  std::vector<ConversationTurn> turns;
  /** Every chirp of every turn, in the order they were placed. */
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
