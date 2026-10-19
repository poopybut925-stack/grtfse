#pragma once
// Predictive Safe Click: independent and side-effect-free geometry/physics oracle.
// NOT GD's exact physics. Only classic, upright cube; static objects; no triggers.
// All lengths and velocities are WORLD PIXELS PER PHYSICS TICK.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace safeclick {
struct Vec { double x = 0, y = 0; };
struct Rect {
    double left = 0, bottom = 0, right = 0, top = 0;
    [[nodiscard]] bool valid() const { return right > left && top > bottom; }
};
inline bool overlaps(Rect a, Rect b) {
    return a.left < b.right && a.right > b.left && a.bottom < b.top && a.top > b.bottom;
}
inline Rect expanded(Rect r, double d) {
    return {r.left - d, r.bottom - d, r.right + d, r.top + d};
}

enum class ObjectKind { Block, RectHazard, TriangleHazard, CircleHazard, Unsupported };
struct Object {
    ObjectKind kind = ObjectKind::Unsupported;
    Rect bounds;
    std::array<Vec, 3> triangle{}; // used for TriangleHazard
    Vec center{};                  // used for CircleHazard
    double radius = 0;
};
struct World {
    // Set true only if surrounding geometry has been EXHAUSTIVELY collected.
    // An incomplete scene must not authorize automatic input.
    bool sceneComplete = false;
    double floorY = 0;
    std::vector<Object> objects;
};
struct Player {
    Vec position{};     // CENTER
    Vec halfSize{.45, .45};
    double vx = 0;      // pixels / tick, independent of render FPS
    double vy = 0;
    double gravity = 0.6; // positive downward acceleration / tick
    double jumpVy = 5;    // upward impulse / tick
    double maxFallSpeed = 20;
    bool grounded = false;
};
struct Options {
    int lookaheadTicks = 24;
    int maxDeferredTicks = 6;
    double safetyMargin = 0; // extra collision padding; conservative
    double maxMicrostepDistance = 1.5;
    int maxMicrosteps = 80;
};
enum class Fate { Survived, Died, Unknown, InvalidPress };
struct Rollout {
    Fate fate = Fate::Unknown;
    int deathTick = -1;
    bool jumpExecuted = false;
    Player endPlayer{};
};
enum class Decision { PressNow, Defer, NoViableWindow, Unknown };
struct Plan {
    Decision decision = Decision::Unknown;
    int delayTicks = -1;           // 0 = now, >0 = future candidate
    std::vector<Fate> candidates; // candidate offsets 0..maxDeferredTicks
};

inline Rect playerBox(const Player& p) {
    return {p.position.x - p.halfSize.x, p.position.y - p.halfSize.y,
            p.position.x + p.halfSize.x, p.position.y + p.halfSize.y};
}
inline bool rectCircle(Rect r, Vec c, double radius) {
    auto x = std::clamp(c.x, r.left, r.right);
    auto y = std::clamp(c.y, r.bottom, r.top);
    double dx = c.x - x, dy = c.y - y;
    return dx * dx + dy * dy < radius * radius;
}
inline void project(const std::array<Vec, 3>& t, Vec axis, double& lo, double& hi) {
    lo = hi = t[0].x * axis.x + t[0].y * axis.y;
    for (int i = 1; i < 3; ++i) {
        double p = t[i].x * axis.x + t[i].y * axis.y;
        lo = std::min(lo, p); hi = std::max(hi, p);
    }
}
// Separating axis test: triangle vs (potentially padded) axis-aligned player.
inline bool rectTriangle(Rect r, const std::array<Vec, 3>& tri) {
    auto separated = [&](Vec axis) {
        double a, b;
        project(tri, axis, a, b);
        double lo = (axis.x >= 0 ? r.left : r.right) * axis.x +
                    (axis.y >= 0 ? r.bottom : r.top) * axis.y;
        double hi = (axis.x >= 0 ? r.right : r.left) * axis.x +
                    (axis.y >= 0 ? r.top : r.bottom) * axis.y;
        return b <= lo || hi <= a;
    };
    if (separated({1, 0}) || separated({0, 1})) return false;
    for (int i = 0; i < 3; ++i) {
        Vec a = tri[i], b = tri[(i + 1) % 3];
        if (separated({a.y - b.y, b.x - a.x})) return false;
    }
    return true;
}
inline bool hits(Rect player, const Object& obj, double margin) {
    Rect padded = expanded(player, margin);
    if (!overlaps(padded, expanded(obj.bounds, margin))) return false;
    switch (obj.kind) {
        case ObjectKind::RectHazard: return true;
        case ObjectKind::TriangleHazard: return rectTriangle(padded, obj.triangle);
        case ObjectKind::CircleHazard: return rectCircle(padded, obj.center, obj.radius + margin);
        case ObjectKind::Block: return false; // physics collision resolved separately
        case ObjectKind::Unsupported: return false;
    }
    return false;
}
inline bool validScene(const Player& p, const World& w, const Options& o) {
    if (!w.sceneComplete || !std::isfinite(w.floorY) ||
        !std::isfinite(p.position.x) || !std::isfinite(p.position.y) ||
        !std::isfinite(p.vx) || !std::isfinite(p.vy) ||
        !std::isfinite(p.halfSize.x) || !std::isfinite(p.halfSize.y) ||
        !std::isfinite(p.gravity) || !std::isfinite(p.jumpVy) ||
        !std::isfinite(p.maxFallSpeed) ||
        p.halfSize.x <= 0 || p.halfSize.y <= 0 || p.gravity <= 0 ||
        p.jumpVy <= 0 || p.maxFallSpeed <= 0 ||
        o.lookaheadTicks < 1 || o.lookaheadTicks > 240 ||
        o.maxDeferredTicks < 0 || o.maxDeferredTicks > 24 ||
        !std::isfinite(o.safetyMargin) || !std::isfinite(o.maxMicrostepDistance) ||
        o.safetyMargin < 0 || o.maxMicrostepDistance <= 0 || o.maxMicrosteps < 1)
        return false;
    for (const auto& obj : w.objects) {
        if (!std::isfinite(obj.bounds.left) || !std::isfinite(obj.bounds.right) ||
            !std::isfinite(obj.bounds.top) || !std::isfinite(obj.bounds.bottom) ||
            !obj.bounds.valid()) return false;
        if (obj.kind == ObjectKind::TriangleHazard) {
            for (const auto& pt : obj.triangle)
                if (!std::isfinite(pt.x) || !std::isfinite(pt.y)) return false;
        }
        if (obj.kind == ObjectKind::CircleHazard &&
            (obj.radius <= 0 || !std::isfinite(obj.radius) ||
             !std::isfinite(obj.center.x) || !std::isfinite(obj.center.y))) return false;
    }
    return true;
}
// Run one physics tick. Uses subdivisions to catch fast crossing hazards.
// Ground and static platforms are supported; slopes, pads, portals, moving
// surfaces, dynamic triggers and inverted gravity are explicitly unsupported.
inline Fate advance(Player& p, const World& w, const Options& o) {
    if (!p.grounded) p.vy = std::max(-p.maxFallSpeed, p.vy - p.gravity);
    else p.vy = -p.gravity; // attempt to retain ground contact
    const double travel = std::max(std::abs(p.vx), std::abs(p.vy));
    if (!std::isfinite(travel) || travel / o.maxMicrostepDistance > o.maxMicrosteps)
        return Fate::Unknown;
    const auto n = static_cast<int>(std::ceil(travel / o.maxMicrostepDistance));
    const int micro = std::max(1, n);
    bool groundedThisTick = false;
    for (int i = 0; i < micro; ++i) {
        Player before = p;
        p.position.x += p.vx / micro;
        p.position.y += p.vy / micro;
        Rect prior = playerBox(before);
        Rect next = playerBox(p);
        // Static terrain: descending into the top is a landing. Any side or
        // underside collision is fatal for this limited cube model.
        for (const auto& obj : w.objects) {
            if (obj.kind != ObjectKind::Block) continue;
            auto b = obj.bounds;
            if (!overlaps(expanded(next, o.safetyMargin), b)) continue;
            const bool crossedTop = prior.bottom >= b.top - 1e-6 &&
                                    next.bottom <= b.top && p.vy <= 0 &&
                                    next.left < b.right && next.right > b.left;
            if (crossedTop) {
                p.position.y = b.top + p.halfSize.y;
                p.vy = 0;
                groundedThisTick = true;
                next = playerBox(p);
            } else {
                return Fate::Died;
            }
        }
        if (p.position.y - p.halfSize.y <= w.floorY) {
            p.position.y = w.floorY + p.halfSize.y;
            p.vy = 0;
            groundedThisTick = true;
            next = playerBox(p);
        }
        for (const auto& obj : w.objects) {
            if (obj.kind == ObjectKind::Unsupported) {
                // Proximity alone makes the scene unknowable, even without a
                // collision: could be a portal, orb, pad or moving hazard.
                if (overlaps(expanded(next, 4.0 + o.safetyMargin), obj.bounds))
                    return Fate::Unknown;
                continue;
            }
            if (hits(next, obj, o.safetyMargin)) return Fate::Died;
        }
    }
    p.grounded = groundedThisTick;
    return Fate::Survived;
}

// Candidate: coast delayTicks ticks, press once if actually grounded, then
// continue simulating for a *full* horizon after the press.
inline Rollout simulate(const Player& start, const World& w, const Options& o, int delayTicks) {
    Rollout result;
    result.endPlayer = start;
    if (!validScene(start, w, o) || delayTicks < 0 || delayTicks > o.maxDeferredTicks) return result;
    Player& p = result.endPlayer;
    for (int t = 0; t < o.lookaheadTicks + delayTicks; ++t) {
        if (t == delayTicks) {
            if (!p.grounded) {
                result.fate = Fate::InvalidPress;
                return result;
            }
            p.vy = p.jumpVy;
            p.grounded = false;
            result.jumpExecuted = true;
        }
        Fate status = advance(p, w, o);
        if (status != Fate::Survived) {
            result.fate = status;
            result.deathTick = t;
            return result;
        }
    }
    result.fate = Fate::Survived;
    return result;
}

inline Plan planClick(const Player& start, const World& w, const Options& o) {
    Plan out;
    if (!validScene(start, w, o)) return out;
    out.candidates.reserve(static_cast<size_t>(o.maxDeferredTicks + 1));
    bool unknown = false;
    for (int d = 0; d <= o.maxDeferredTicks; ++d) {
        auto r = simulate(start, w, o, d);
        out.candidates.push_back(r.fate);
        if (r.fate == Fate::Survived) {
            if (unknown) {
                // Earlier candidates were unknown: cannot certify EARLIEST safe tick.
                out.decision = Decision::Unknown;
                return out;
            }
            out.delayTicks = d;
            out.decision = d == 0 ? Decision::PressNow : Decision::Defer;
            return out;
        }
        if (r.fate == Fate::Unknown) unknown = true;
    }
    out.decision = unknown ? Decision::Unknown : Decision::NoViableWindow;
    return out;
}
} // namespace safeclick
