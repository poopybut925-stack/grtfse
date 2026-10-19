#pragma once
#include "Predictor.hpp"
#include "SafeClickBuffer.hpp"

namespace safeclick {
// Thread-unsafe by design: all calls must come from one gameplay/physics thread.
// Scene acquisition and GD's input dispatch remain the responsibility of the
// Geode adapter. This class never calls the live engine.
class Controller {
public:
    explicit Controller(Options options = {}) : options_(options) {}
    Events physicalDown(const Player& player, const World& world) {
        lastPlan_ = planClick(player, world, options_);
        return buffer_.press(initialVerdict(lastPlan_.decision), options_.maxDeferredTicks);
    }
    Events physicalUp() { return buffer_.release(); }
    Events nextPhysicsTick(const Player& player, const World& world) {
        // For a click already buffered, the new observation must affirm that
        // NOW is safe. Never fire simply because the original plan predicted
        // tick +N would be safe; hazards/triggers may have changed meanwhile.
        if (!buffer_.pending()) return buffer_.tick(Safety::Unknown);
        lastPlan_ = planClick(player, world, options_);
        const auto verdict = lastPlan_.decision == Decision::PressNow
            ? Safety::Safe : Safety::Unsafe;
        return buffer_.tick(verdict);
    }
    Events cancel() { return buffer_.cancel(); }
    const Plan& lastPlan() const { return lastPlan_; }
    bool pending() const { return buffer_.pending(); }
    bool physicalHeld() const { return buffer_.physicalHeld(); }
private:
    static Safety initialVerdict(Decision d) {
        switch (d) {
            case Decision::PressNow: return Safety::Safe;
            case Decision::Defer: case Decision::NoViableWindow: return Safety::Unsafe;
            case Decision::Unknown: return Safety::Unknown;
        }
        return Safety::Unknown;
    }
    Options options_;
    Buffer buffer_;
    Plan lastPlan_;
};
} // namespace safeclick
