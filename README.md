# Predictive Safe Click v0.3 — **experimental Windows Geode mod project**

**This ZIP is NOT itself an installable .geode. It is a real Geode source project with a first live-game adapter, CMake configuration and a GitHub Actions Windows build workflow.** The adapter is written but **has not been compiled against an installed Geode SDK or tested in GD**. Its simplified physics approximation is not Eclipse's trajectory engine. Eclipse was researched as a reference; no Eclipse copyrighted code is included.

Target: GD **2.2081**, Windows x64, Geode **5.3+**. Compatible with GD 2.2081 if the bindings match, not other GD releases.

## What's actually new versus the standalone v0.2 engine?

- `src/LiveAdapter.cpp` hooks **real gameplay input** (`GJBaseGameLayer::handleButton`) and **command ticks** (`GJBaseGameLayer::processCommands`), using Geode's public modification framework.
- Reads the actual P1 cube's world position and grounded state, nearby `PlayLayer::m_objects` collision rectangles and object types.
- `src/LiveCalibration.hpp` fits a crude horizontal speed, vertical acceleration, and jump impulse from the last few **observed game ticks**, instead of imposing constants from the demonstration.
- Uses `src/Predictor.hpp` to try click delays 0 through N against predicted collisions and routes suppression and delayed press/release through `src/Controller.hpp`.
- Unknown/incomplete geometry and unsupported modes cause native input passthrough or conservative cancellation. Experimental correction is **OFF by default**. Use practice mode to test.
- Logs buffered and fired presses to Geode log. Inputs are released on reset/quit and buffer expiry.
- Includes standalone C++ tests for prediction, input timing and calibration.

### Important limitations

- This is a **first unverified live adapter**, not a certified cheat. It may fail to build if the SDK's current bindings have different names/signatures. In that case, send the compiler error so the exact binding can be fixed.
- GD's precise physics, rotated/triangle/circular hitboxes, portals, triggers, pads, moving objects, orbs, dual mode, and other gamemodes are not modeled. Collision rectangles are estimates, not authoritative. **It may give false positives or delay the wrong click.**
- It only attempts predictive correction on **upright, normal-size, grounded cube** in **simple levels with static hazards and blocks** after calibration. Short inputs are deferred even if physically released; the synthetic press lasts one physics tick.
- `m_objects` might not include every relevant or active object. A lack of visible objects does **not** imply the future is known. Do not use on meaningful progression before verifying against repeated practice tests.
- As implemented, only the first observed jump calibrates enough for the assist, so a fresh attempt may not be assisted immediately. Calibration currently resets with the attempt.
- Running at 240 FPS does not set physics tick length. Model values are learned from command callbacks, not refresh rate.
- Disable other replay/input-hooking mods when testing, especially Click Between Frames, ZBot, or macro tooling. Your separately installed Eclipse has its own hooks and could affect behavior.

## Best route to a real `.geode` using GitHub Actions (no Windows SDK setup)

1. Upload the *contents* of this folder (including hidden `.github/workflows/build-windows.yml`) to a new GitHub repository. The GitHub app connector can help with that when connected; otherwise use a normal repository upload.
2. Open that repository's **Actions** tab, select **Build Windows .geode**, then **Run workflow**.
3. Wait for build results. If successful, open the completed run's **Artifacts** and download `SafeClick-Windows-geode`.
4. Extract the artifact ZIP; inside should be the genuine `.geode` package produced by the official Geode builder. Install only that `.geode` file in Geometry Dash's Geode mods folder.
5. If the build FAILS, copy the compiler error output back into our chat. That is an expected possibility and is precisely what must be fixed before any in-game test.

This workflow invokes [Geode's official build-geode-mod action](https://github.com/geode-sdk/build-geode-mod); **the build workflow hasn't yet been run**.

## Alternative: compile locally on Windows

You need the Geode CLI, SDK, compiler (Visual Studio Build Tools with Windows SDK, or Clang + Windows SDK), CMake and Ninja. See the [Geode development guide](https://github.com/geode-sdk/docs/tree/main/getting-started). In PowerShell, in this extracted directory, run:

```
winget install GeodeSDK.GeodeCLI
geode sdk install
geode sdk install-binaries
```

Restart PowerShell so `$env:GEODE_SDK` is refreshed. Then run `./build_windows.ps1`; find the `.geode` under `build/`. The script intentionally prints compilation failures.

## Standalone tests (cross-platform)

```
cmake -S . -B build-tests -DSAFECLICK_BUILD_TESTS=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

At packaging time, all **3 standalone test suites passed** on Linux. This does not test the Geode adapter, game hook, or any in-game behavior.

## Code layout

- `src/LiveAdapter.cpp` — input hook + level observation + packet dispatch.
- `src/LiveCalibration.hpp` — empirical velocity/gravity/impulse estimation.
- `src/Predictor.hpp` — synthetic collision and trajectory candidate evaluator.
- `src/Controller.hpp` and `src/SafeClickBuffer.hpp` — input state and held/released tap behavior.
- `tests/*.cpp` — dependency-free standalone unit tests.
- `.github/workflows/build-windows.yml` — official Geode builder on Windows runner.

This project is independent experimental code, not a fork of Eclipse Menu. No guarantee it loads or works correctly until a real game test has been done.
