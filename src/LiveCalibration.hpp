#pragma once
#include <algorithm>
#include <cmath>

namespace safeclick {
// Samples MUST come from consecutive 240Hz processCommands callbacks.
// No live player state is altered by this recorder.
class LiveCalibration {
public:
    void reset() { *this = {}; }
    void observe(double x, double y, bool grounded) {
        if (!std::isfinite(x) || !std::isfinite(y)) { reset(); return; }
        if (!haveLast_) { prevX_ = x; prevY_ = y; prevGrounded_ = grounded; haveLast_ = true; return; }
        double dx = x - prevX_, dy = y - prevY_;
        if (dx < -3 || std::abs(dx) > 30 || std::abs(dy) > 35) {
            // Reset/checkpoint/portal/speed shift: invalidate old model.
            reset(); prevX_ = x; prevY_ = y; prevGrounded_ = grounded; haveLast_ = true;
            return;
        }
        if (dx > 0.02 && dx < 15) {
            if (horizontalSamples_ && std::abs(dx - vx_) > std::max(0.35, vx_ * .3))
                horizontalSamples_ = 0; // likely changed speed
            vx_ = horizontalSamples_ ? (.88 * vx_ + .12 * dx) : dx;
            horizontalSamples_ = std::min(horizontalSamples_ + 1, 500);
        }
        if (!grounded && !prevGrounded_ && havePrevDy_) {
            double g = prevDy_ - dy;
            if (g > .001 && g < 1.5 && dy > -15) {
                grav_ = gravitySamples_ ? (.9 * grav_ + .1 * g) : g;
                gravitySamples_ = std::min(gravitySamples_ + 1, 500);
            }
        }
        if (prevGrounded_ && !grounded && dy > .03) {
            jump_ = dy;
            jumpSamples_++;
        } else if (!grounded && dy > jump_ && dy < 15) {
            jump_ = dy;
        }
        prevDy_ = dy;
        havePrevDy_ = !grounded;
        prevGrounded_ = grounded;
        prevX_ = x;
        prevY_ = y;
    }
    bool ready() const {
        return horizontalSamples_ >= 10 && gravitySamples_ >= 5 &&
               jumpSamples_ >= 1 && vx_ > .02 && jump_ > .03 && grav_ > .001;
    }
    double vx() const { return vx_; }
    double gravity() const { return grav_; }
    double jumpVelocity() const { return jump_; }
    int horizontalSamples() const { return horizontalSamples_; }
    int gravitySamples() const { return gravitySamples_; }
private:
    bool haveLast_ = false, prevGrounded_ = true, havePrevDy_ = false;
    int horizontalSamples_ = 0, gravitySamples_ = 0, jumpSamples_ = 0;
    double prevX_ = 0, prevY_ = 0, prevDy_ = 0;
    double vx_ = 0, grav_ = 0, jump_ = 0;
};
} // namespace safeclick
