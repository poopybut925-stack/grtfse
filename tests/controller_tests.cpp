#include "../src/Controller.hpp"
#include "../src/Calibration.hpp"
#include <cstdlib>
#define CHECK(expr) do { if (!(expr)) { std::cerr << "FAIL CHECK " #expr << " (" << __FILE__ << ":" << __LINE__ << ")\n"; std::abort(); } } while(false)
#include <iostream>
using namespace safeclick;

int main() {
    Options o; o.lookaheadTicks=18; o.maxDeferredTicks=6;
    World w; w.sceneComplete=true; w.floorY=0;
    Object hazard; hazard.kind=ObjectKind::RectHazard;
    hazard.bounds={1,1.5,3,3.5}; w.objects.push_back(hazard);
    Player p; p.position={0,.5};p.halfSize={.5,.5};p.vx=2;
    p.vy=0;p.gravity=.6;p.jumpVy=5;p.grounded=true;
    Controller c(o);
    CHECK(c.lastPlan().decision==Decision::Unknown);
    auto first=c.physicalDown(p,w);
    CHECK(!first.press && c.pending());
    CHECK(c.lastPlan().decision==Decision::Defer);
    CHECK(c.lastPlan().delayTicks==2);
    c.physicalUp();
    CHECK(advance(p,w,o)==Fate::Survived);
    CHECK(!c.nextPhysicsTick(p,w).press);
    CHECK(advance(p,w,o)==Fate::Survived);
    CHECK(c.nextPhysicsTick(p,w).press);
    CHECK(c.lastPlan().decision==Decision::PressNow);
    CHECK(c.nextPhysicsTick(p,w).release);
    std::cout<<"PASS live reevaluation: 100 suppress -> 101 wait -> 102 fire -> 103 release\n";

    {
        // Trace generated from the same simple model (1 tick per sample).
        Player q; q.position={0,.5}; q.halfSize={.5,.5};q.vx=2;q.vy=0;
        q.gravity=.6;q.jumpVy=5;q.grounded=true;
        World empty;empty.sceneComplete=true;
        std::vector<Sample> samples{{10,q.position,q.grounded}};
        q.vy=q.jumpVy;q.grounded=false;
        for (int tick=11;tick<=17;++tick) {
            CHECK(advance(q,empty,o)==Fate::Survived);
            samples.push_back({tick,q.position,q.grounded});
        }
        auto fitted=calibrate(samples,.5);
        CHECK(fitted.has_value());
        CHECK(std::abs(fitted->gravity-.6)<1e-9);
        CHECK(std::abs(fitted->jumpVy-5)<1e-9);
        CHECK(std::abs(fitted->vx-2)<1e-9);
        std::cout<<"PASS calibration estimates jump speed, gravity, horizontal speed\n";
    }
    {
        std::vector<Sample> bad{{0,{0,0},true},{2,{2,1},false}};
        CHECK(!calibrate(bad,.5));
        std::cout<<"PASS calibration refuses missing tick data\n";
    }
    std::cout<<"ALL CONTROLLER TESTS PASSED\n";
}
