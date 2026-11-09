/**
 * @file HeatCurve.h
 * @brief The heat of one conversation over time: a bell curve with a small wobble.
 *
 * Heat runs from 0 (cold) to 1 (hot). The curve is fully determined by the values it is built
 * from, so the same values always give the same heat at any moment. `draw()` builds a curve whose
 * values are drawn from the ranges in the settings.
 */
#pragma once

#include <random>

#include "ConversationConfig.h"

class HeatCurve {
 public:
  /**
   * @param peakHeat Heat at the top of the bell, 0 to 1.
   * @param peakTimeSeconds When the peak falls, in seconds from the start.
   * @param widthSeconds Width of the bell (its standard deviation), in seconds; above 0.
   * @param baselineHeat Heat the curve never drops below at the edges, 0 to 1.
   * @param wobbleAmount Size of the wobble added to the curve, in heat units.
   * @param wobblePeriodSeconds Period of the slowest wobble, in seconds.
   * @param fastWobbleRatio How many times faster the second wobble runs than the slow one.
   * @param wobblePhaseSlow Starting phase of the slow wobble, in radians.
   * @param wobblePhaseFast Starting phase of the faster wobble, in radians.
   */
  HeatCurve(double peakHeat, double peakTimeSeconds, double widthSeconds, double baselineHeat,
            double wobbleAmount, double wobblePeriodSeconds, double fastWobbleRatio,
            double wobblePhaseSlow, double wobblePhaseFast);

  /**
   * Builds a curve with values drawn from the settings' ranges.
   *
   * @param config Where the ranges come from.
   * @param durationSeconds Length of the conversation; the peak time and width scale with it.
   * @param rng Source of randomness; consumed.
   */
  static HeatCurve draw(const HeatConfig& config, double durationSeconds, std::mt19937& rng);

  /** Heat at `timeSeconds` from the start, clamped to 0..1. */
  double heatAt(double timeSeconds) const;

  double peakHeat() const { return peakHeat_; }
  double peakTimeSeconds() const { return peakTimeSeconds_; }
  double widthSeconds() const { return widthSeconds_; }

 private:
  double peakHeat_;
  double peakTimeSeconds_;
  double widthSeconds_;
  double baselineHeat_;
  double wobbleAmount_;
  double wobblePeriodSeconds_;
  double fastWobbleRatio_;
  double wobblePhaseSlow_;
  double wobblePhaseFast_;
};
