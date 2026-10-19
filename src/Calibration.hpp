#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>
#include "Predictor.hpp"

namespace safeclick {
// Fit an elementary constant-acceleration model from a clean upright cube
// jump sampled once per *actual physics tick*, with no pads/portals/triggers.
// The very first sample is grounded immediately BEFORE the jump, followed by
// consecutive airborne samples. Do not feed samples taken per render frame.
struct Sample { int physicsTick; Vec center; bool grounded; };
struct Calibration { double vx, gravity, jumpVy, floorY; double yResidual; };
inline std::optional<Calibration> calibrate(const std::vector<Sample>& samples,
                                           double halfHeight) {
    if (samples.size() < 5 || halfHeight <= 0 || !samples[0].grounded)
        return std::nullopt;
    for (size_t i=1;i<samples.size();++i) {
        if (samples[i].physicsTick != samples[i-1].physicsTick + 1 ||
            samples[i].grounded || !std::isfinite(samples[i].center.x) ||
            !std::isfinite(samples[i].center.y)) return std::nullopt;
    }
    std::vector<double> dd;
    std::vector<double> dx;
    std::vector<double> dy;
    for (size_t i=1; i<samples.size(); ++i) {
        dx.push_back(samples[i].center.x - samples[i-1].center.x);
        dy.push_back(samples[i].center.y - samples[i-1].center.y);
    }
    for (size_t i=1; i<dy.size(); ++i) dd.push_back(dy[i-1] - dy[i]);
    std::sort(dd.begin(),dd.end());
    std::sort(dx.begin(),dx.end());
    double g=dd[dd.size()/2], vx=dx[dx.size()/2];
    if (g <= 0 || g > 25 || !std::isfinite(vx)) return std::nullopt;
    double jumpVy=dy[0]+g;
    if (jumpVy <= 0 || jumpVy > 100) return std::nullopt;
    double residual=0;
    for (size_t i=0; i<dy.size(); ++i) {
        residual=std::max(residual,std::abs(dy[i]-(jumpVy - g*(i+1))));
    }
    if (residual > .05 * std::max(1.0,jumpVy)) return std::nullopt;
    return Calibration{vx,g,jumpVy,samples[0].center.y-halfHeight,residual};
}
} // namespace safeclick
