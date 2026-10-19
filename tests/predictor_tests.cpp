#include "../src/Predictor.hpp"
#include "../src/SafeClickBuffer.hpp"
#include <cstdlib>
#define CHECK(expr) do { if (!(expr)) { std::cerr << "FAIL CHECK " #expr << " (" << __FILE__ << ":" << __LINE__ << ")\n"; std::abort(); } } while(false)
#include <iostream>
#include <limits>
using namespace safeclick;

World emptyWorld() { World w; w.sceneComplete = true; return w; }
Player cube() { Player p; p.position={0,.5}; p.halfSize={.5,.5}; p.vx=2; p.gravity=.6; p.jumpVy=5; p.grounded=true; return p; }
Object rectHazard(double l, double b, double r, double t) {
    return {ObjectKind::RectHazard,{l,b,r,t}};
}
Object block(double l, double b, double r, double t) {
    return {ObjectKind::Block,{l,b,r,t}};
}
void test(const char* title, bool ok) { if (!ok) { std::cerr << "FAIL " << title << "\n"; std::abort(); } std::cout << "PASS " << title << "\n"; }

int main() {
    Options o; o.lookaheadTicks=18; o.maxDeferredTicks=6;
    {
        World w=emptyWorld();
        auto p = cube();
        auto r=simulate(p,w,o,0);
        test("Empty floor: jump executes and survives", r.fate==Fate::Survived && r.jumpExecuted);
        test("Empty floor: press immediately", planClick(p,w,o).decision==Decision::PressNow);
    }
    {
        auto w=emptyWorld(); auto p=cube();
        w.sceneComplete=false;
        test("Missing geometry: fail unknown", planClick(p,w,o).decision==Decision::Unknown);
        w.sceneComplete=true; w.objects.push_back({ObjectKind::Unsupported, {1,0,5,5}});
        test("Unknown object in path: no false safe", planClick(p,w,o).decision==Decision::Unknown);
    }
    {
        auto w=emptyWorld(); auto p=cube();
        w.objects.push_back(rectHazard(2,0,100,6));
        test("Unavoidable hazard: no viable window", planClick(p,w,o).decision==Decision::NoViableWindow);
    }
    {
        auto w=emptyWorld(); auto p=cube();
        // Prove the geometry engine can find actual delayed safe jump windows.
        // Search combinations of static overhead-hazard locations, NOT a mocked oracle.
        bool found=false; Plan sample;
        for (double left=1; left<=23 && !found; left+=1) {
            for (double bottom=1.5; bottom<=15 && !found; bottom+=.5) {
                for (double width=1; width<=14 && !found; width+=1) {
                    w.objects={rectHazard(left,bottom,left+width,bottom+2)};
                    auto pl=planClick(p,w,o);
                    if (pl.decision==Decision::Defer && pl.delayTicks>=2) {
                        found=true; sample=pl; std::cout << "  Geometry: hazard ("<<left<<","<<bottom<<") to ("<<(left+width)<<","<<(bottom+2)<<")\n";
                    }
                }
            }
        }
        test("Physics finds deferred safe window for static geometry",found);
        if (found) {
            for (int t=0;t<sample.delayTicks;t++)
                test("Earlier click dies (real simulation)",sample.candidates[t]==Fate::Died);
            test("Chosen click survives",sample.candidates[sample.delayTicks]==Fate::Survived);
            std::cout<<"  First eligible offset = +"<<sample.delayTicks<<" ticks\n";
        }
    }
    {
        auto w=emptyWorld();
        Object tri; tri.kind=ObjectKind::TriangleHazard;
        tri.triangle={Vec{4,0},Vec{6,0},Vec{5,3}};tri.bounds={4,0,6,3};
        w.objects={tri};
        test("Triangle SAT detects spike collision",hits({4.9,1,5.1,1.3},tri,0));
        test("Triangle SAT permits clear space",!hits({3,1,3.2,1.3},tri,0));
    }
    {
        auto w=emptyWorld();
        Object cir;cir.kind=ObjectKind::CircleHazard;cir.center={3,4};cir.radius=2;
        cir.bounds={1,2,5,6};w.objects={cir};
        test("Circle vs rectangle hazard",hits({2,2,2.5,2.5},cir,0));
    }
    {
        auto w=emptyWorld();auto p=cube(); p.vx=500;
        test("Too much speed for collision sampling: unknown",
            planClick(p,w,o).decision==Decision::Unknown);
    }
    {
        auto w=emptyWorld(); auto p=cube();
        w.objects={block(5,0,8,40)};
        // Running into a wall causes death; no magical pass-through.
        test("Solid side collision kills",simulate(p,w,o,0).fate==Fate::Died);
    }
    {
        Buffer b;
        test("Press at tick 100 suppressed",!b.press(Safety::Unsafe,6).press);
        b.release();
        test("Tick 101 unsafe",!b.tick(Safety::Unsafe).press);
        test("Tick 102 unsafe",!b.tick(Safety::Unsafe).press);
        test("Tick 103 safe fires delayed tap",b.tick(Safety::Safe).press);
        test("Tick 104 releases tap",b.tick(Safety::Unknown).release);
    }
    std::cout << "ALL PREDICTOR TESTS PASSED\n";
}
