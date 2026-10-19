#include "../src/LiveCalibration.hpp"
#include <cassert>
#include <cstdio>
int main() {
    safeclick::LiveCalibration m;
    double x = 0, y = 11.5;
    for (int i = 0; i < 30; ++i) { x += 1.2; m.observe(x,y,true); }
    assert(!m.ready());
    double vy = 1.7;
    for (int i = 0; i < 30; ++i) {
        x += 1.2; y += vy; vy -= .10;
        m.observe(x,y,false);
    }
    assert(m.ready());
    assert(m.vx() > 1.1 && m.vx() < 1.3);
    assert(m.gravity() > .08 && m.gravity() < .13);
    assert(m.jumpVelocity() > 1.55 && m.jumpVelocity() < 1.75);
    m.reset();
    assert(!m.ready());
    std::puts("live_calibration_tests: passed");
}
