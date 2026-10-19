// Predictive Safe Click, first live Geode adapter for GD 2.2081.
// An experimental model, NOT an Eclipse trajectory clone. Does not call
// GD's collision functions on a simulated player; no real-player rewinds.
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include "Predictor.hpp"
#include "Controller.hpp"
#include "LiveCalibration.hpp"

using namespace geode::prelude;
namespace {
    safeclick::Controller s_controller;
    safeclick::LiveCalibration s_calibration;
    bool s_injecting = false;
    bool s_floorKnown = false;
    double s_floorY = 0;
    int s_tick = 0;
    int s_queuedAt = -1;
    bool s_nativeHeld = false;

    bool settingBool(char const* key) {
        return Mod::get()->getSettingValue<bool>(key);
    }
    int settingInt(char const* key, int low, int high) {
        return static_cast<int>(std::clamp<int64_t>(
            Mod::get()->getSettingValue<int64_t>(key), low, high));
    }
    void clearState() {
        s_controller.cancel();
        s_calibration.reset();
        s_floorKnown = false;
        s_tick = 0;
        s_queuedAt = -1;
        s_nativeHeld = false;
    }
    bool compatibleCube(PlayLayer* pl) {
        if (!pl || pl->m_playerDied || pl->m_isPaused || !pl->m_player1)
            return false;
        auto* p = pl->m_player1;
        if (p->m_isDead || p->m_controlsDisabled || p->m_inputsLocked ||
            p->m_isPlatformer || p->m_isShip || p->m_isBird || p->m_isDart ||
            p->m_isBall || p->m_isSpider || p->m_isRobot || p->m_isSwing ||
            p->m_isUpsideDown || p->m_isOnSlope || p->m_vehicleSize < .95f)
            return false;
        return true;
    }
    bool active(PlayLayer* pl) {
        return settingBool("enabled") && compatibleCube(pl) &&
               (!settingBool("practice-only") || pl->m_isPracticeMode);
    }
    safeclick::Options makeOptions() {
        safeclick::Options options;
        options.maxDeferredTicks = settingInt("buffer-ticks", 1, 16);
        options.lookaheadTicks = settingInt("lookahead-ticks", 8, 80);
        options.safetyMargin = 1.1; // approximations favor safety over precision
        options.maxMicrostepDistance = .75;
        options.maxMicrosteps = 128;
        return options;
    }
    // Expected game convention: player is centered on its world position.
    // GD's actual hitbox is NOT the exact visual sprite size, and this 11.5px
    // approximation needs validation against recorded deaths.
    constexpr double cubeHalfWidth = 11.5;
    constexpr double cubeHalfHeight = 11.5;

    std::optional<std::pair<safeclick::Player, safeclick::World>> snapshot(PlayLayer* pl) {
        if (!compatibleCube(pl) || !s_calibration.ready() || !s_floorKnown ||
            !pl->m_objects) return std::nullopt;
        auto* p = pl->m_player1;
        const auto xy = p->getPosition();
        const auto opts = makeOptions();
        safeclick::Player current;
        current.position = {xy.x, xy.y};
        current.halfSize = {cubeHalfWidth, cubeHalfHeight};
        current.vx = s_calibration.vx();
        current.vy = 0; // assigned only to grounded player below
        current.gravity = s_calibration.gravity();
        current.jumpVy = s_calibration.jumpVelocity();
        current.maxFallSpeed = std::max(3., current.jumpVy * 3.);
        current.grounded = p->m_isOnGround || p->m_isOnGround2;
        if (!current.grounded) {
            // Deriving mid-air Y velocity safely needs a per-tick accessor.
            // For now we cannot predict a new grounded click from mid-air.
            return std::nullopt;
        }
        safeclick::World world;
        world.floorY = s_floorY;
        world.sceneComplete = true;
        const double furthestX = xy.x + (opts.lookaheadTicks + opts.maxDeferredTicks + 2)
                                                  * current.vx + 35.;
        const double xLeft = xy.x - 40.;
        // Do not claim survival if a changing portal/pad/trigger or an unsupported
        // solid/hazard is nearby. The polygon/circle model is conservative.
        int objectsSeen = 0;
        for (auto* item : geode::cocos::CCArrayExt<GameObject*>(pl->m_objects)) {
            if (!item) continue;
            if (++objectsSeen > 85000) { world.sceneComplete = false; break; }
            const auto xyObj = item->getPosition();
            if (xyObj.x < xLeft || xyObj.x > furthestX ||
                xyObj.y < s_floorY - 35. || xyObj.y > s_floorY + 210.) continue;
            if (std::abs(item->getRotation()) > .01f) {
                world.sceneComplete = false;
                break;
            }
            // GameObject::getObjectRect uses actual gameplay bounds rather than
            // sprite bounds where the object's collision rectangle is defined.
            const cocos2d::CCRect box = item->getObjectRect();
            if (box.size.width <= 0 || box.size.height <= 0) {
                world.sceneComplete = false;
                break;
            }
            safeclick::Object ob;
            ob.bounds = {box.getMinX(), box.getMinY(), box.getMaxX(), box.getMaxY()};
            if (item->m_objectType == GameObjectType::Solid) {
                // Current 2D model resolves top landings, not slopes.
                ob.kind = safeclick::ObjectKind::Block;
            } else if (item->m_objectType == GameObjectType::Hazard) {
                // Enclosing rectangle is conservative for spikes; later replace
                // with real GD hitbox/polygon/circle classification.
                ob.kind = safeclick::ObjectKind::RectHazard;
            } else {
                // Unknown potentially interactive geometry nearby: fail open.
                world.sceneComplete = false;
                break;
            }
            world.objects.push_back(ob);
        }
        return std::make_pair(current, std::move(world));
    }
    void dispatch(GJBaseGameLayer* layer, safeclick::Events e) {
        if (!e.press && !e.release) return;
        s_injecting = true;
        if (e.release) layer->GJBaseGameLayer::handleButton(false, 1, true);
        if (e.press) layer->GJBaseGameLayer::handleButton(true, 1, true);
        s_injecting = false;
    }
    void cancel(GJBaseGameLayer* layer) {
        dispatch(layer, s_controller.cancel());
        s_queuedAt = -1;
    }
}

class $modify(SafeClickPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        clearState();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }
    void resetLevel() {
        cancel(this);
        clearState();
        PlayLayer::resetLevel();
    }
    void onQuit() {
        cancel(this);
        clearState();
        PlayLayer::onQuit();
    }
    void onExit() {
        cancel(this);
        clearState();
        PlayLayer::onExit();
    }
};

class $modify(SafeClickBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto* pl = PlayLayer::get();
        if (s_injecting || this != pl || button != 1 || !isPlayer1) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }
        if (!active(pl)) {
            cancel(this);
            s_nativeHeld = down;
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }
        // Do not buffer when the oracle has no calibrated model.
        safeclick::Events events;
        if (down) {
            if (s_nativeHeld) {
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
                return;
            }
            if (s_controller.physicalHeld()) return; // key auto-repeat
            if (!s_controller.pending()) {
                s_controller = safeclick::Controller(makeOptions());
            }
            auto observation = snapshot(pl);
            if (!observation) {
                // Unknown scenario = native game input, no suppression.
                // We must also forward the matching release (not consume it).
                s_nativeHeld = true;
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
                return;
            }
            events = s_controller.physicalDown(observation->first, observation->second);
            if (s_controller.pending()) {
                s_queuedAt = s_tick;
                if (settingBool("log-decisions")) log::info("Click buffered at physics tick {}", s_tick);
            }
        } else {
            if (s_nativeHeld) {
                s_nativeHeld = false;
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
                return;
            }
            events = s_controller.physicalUp();
        }
        dispatch(this, events);
    }
    void processCommands(float dt, bool p1, bool p2) {
        auto* pl = PlayLayer::get();
        if (this == pl) {
            ++s_tick;
            if (!active(pl)) {
                cancel(this);
            } else {
                if (s_controller.pending()) {
                    auto observation = snapshot(pl);
                    if (observation) {
                        auto events = s_controller.nextPhysicsTick(observation->first, observation->second);
                        if (events.press && settingBool("log-decisions") && s_queuedAt >= 0)
                            log::info("SAVED +{} TICKS: delayed click fired", s_tick - s_queuedAt);
                        dispatch(this, events);
                    } else {
                        // Timer still progresses even if prediction becomes unavailable.
                        // No speculative firing: Unknown must not become Safe.
                        safeclick::World unknown;
                        safeclick::Player player;
                        dispatch(this, s_controller.nextPhysicsTick(player, unknown));
                    }
                } else {
                    // Auto-release a buffered tap after one full physics step.
                    safeclick::World unused;
                    safeclick::Player player;
                    dispatch(this, s_controller.nextPhysicsTick(player, unused));
                }
            }
        }
        GJBaseGameLayer::processCommands(dt, p1, p2);
        if (this == pl && compatibleCube(pl)) {
            auto* p = pl->m_player1;
            bool grounded = p->m_isOnGround || p->m_isOnGround2;
            const auto pos = p->getPosition();
            s_calibration.observe(pos.x, pos.y, grounded);
            if (grounded) {
                s_floorY = pos.y - cubeHalfHeight;
                s_floorKnown = true;
            }
        }
    }
};
