#pragma once
#include <algorithm>

// Engine-independent core. One input button, one player.
// 'Safe' MUST come from an actual future-trajectory predictor, not from
// the player's current collision state. See README before enabling.
namespace safeclick {

enum class Safety { Safe, Unsafe, Unknown };

struct Events {
    bool press = false;
    bool release = false;
};

class Buffer {
public:
    bool physicalHeld() const { return physicalHeld_; }
    bool gameHeld() const { return gameHeld_; }
    bool pending() const { return pending_; }
    int remaining() const { return remaining_; }

    Events press(Safety verdict, int maxTicks) {
        if (physicalHeld_) return {};
        physicalHeld_ = true;
        // If a previous uncommitted click exists, reuse it. A repeated down
        // should never create two game presses.
        if (pending_ || gameHeld_) return {};
        if (verdict != Safety::Unsafe) {
            // Fail open when the oracle knows nothing: don't add input lag.
            gameHeld_ = true;
            return {.press = true};
        }
        pending_ = true;
        remaining_ = std::clamp(maxTicks, 1, 24);
        return {};
    }

    Events release() {
        if (!physicalHeld_) return {};
        physicalHeld_ = false;
        if (pending_) return {}; // save a short tap as a complete future pulse
        if (!gameHeld_) return {};
        gameHeld_ = false;
        return {.release = true};
    }

    Events tick(Safety verdict) {
        // If a buffered tap fired one tick ago, complete its pulse now.
        // Do this even if there is no pending input.
        if (gameHeld_ && !physicalHeld_) {
            gameHeld_ = false;
            return {.release = true};
        }
        if (!pending_) return {};
        if (verdict == Safety::Safe) {
            pending_ = false;
            remaining_ = 0;
            gameHeld_ = true;
            // Held physical input keeps the game button pressed until actual
            // release; a tap auto-releases on the next update.
            return {.press = true};
        }
        // Unsafe or Unknown: neither guesses a safe tick nor fires the click.
        if (--remaining_ <= 0) {
            pending_ = false;
            remaining_ = 0;
        }
        return {};
    }

    // Use when the level resets or the feature is disabled. The integrator
    // must dispatch the release before clearing its own injection guard.
    Events cancel() {
        const bool releaseButton = gameHeld_;
        physicalHeld_ = false;
        gameHeld_ = false;
        pending_ = false;
        remaining_ = 0;
        return {.release = releaseButton};
    }

private:
    bool physicalHeld_ = false;
    bool gameHeld_ = false;
    bool pending_ = false;
    int remaining_ = 0;
};

} // namespace safeclick
