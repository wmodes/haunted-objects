/**
 * @file HeatCurve.cpp
 * @brief Implementation of HeatCurve. See HeatCurve.h.
 */
#include "HeatCurve.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

double drawFrom(const Range& range, std::mt19937& rng) {
  return std::uniform_real_distribution<double>(range.min, range.max)(rng);
}

}  // namespace

HeatCurve::HeatCurve(double peakHeat, double peakTimeSeconds, double widthSeconds,
                     double baselineHeat, double wobbleAmount, double wobblePeriodSeconds,
                     double fastWobbleRatio, double wobblePhaseSlow, double wobblePhaseFast)
    : peakHeat_(peakHeat),
      peakTimeSeconds_(peakTimeSeconds),
      widthSeconds_(widthSeconds),
      baselineHeat_(baselineHeat),
      wobbleAmount_(wobbleAmount),
      wobblePeriodSeconds_(wobblePeriodSeconds),
      fastWobbleRatio_(fastWobbleRatio),
      wobblePhaseSlow_(wobblePhaseSlow),
      wobblePhaseFast_(wobblePhaseFast) {}

HeatCurve HeatCurve::draw(const HeatConfig& config, double durationSeconds, std::mt19937& rng) {
  const double peakHeat = drawFrom(config.peakHeat, rng);
  const double peakTimeSeconds = drawFrom(config.peakTimeFraction, rng) * durationSeconds;
  const double widthSeconds = drawFrom(config.widthFraction, rng) * durationSeconds;
  // Any starting phase is as good as any other: a full turn of the circle.
  const double phaseSlow = drawFrom({0.0, 2.0 * kPi}, rng);
  const double phaseFast = drawFrom({0.0, 2.0 * kPi}, rng);
  return HeatCurve(peakHeat, peakTimeSeconds, widthSeconds, config.baselineHeat,
                   config.wobbleAmount, config.wobblePeriodSeconds, config.fastWobbleRatio,
                   phaseSlow, phaseFast);
}

double HeatCurve::heatAt(double timeSeconds) const {
  const double distanceFromPeak = (timeSeconds - peakTimeSeconds_) / widthSeconds_;
  const double bell = std::exp(-0.5 * distanceFromPeak * distanceFromPeak);
  const double base = baselineHeat_ + (peakHeat_ - baselineHeat_) * bell;

  const double slow = std::sin(2.0 * kPi * timeSeconds / wobblePeriodSeconds_ + wobblePhaseSlow_);
  const double fast = std::sin(
      2.0 * kPi * timeSeconds / (wobblePeriodSeconds_ / fastWobbleRatio_) + wobblePhaseFast_);
  const double wobble = wobbleAmount_ * 0.5 * (slow + fast);

  return std::clamp(base + wobble, 0.0, 1.0);
}
