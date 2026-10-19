Run geode-sdk/build-geode-mod@main
Detect platform
1s
Setup sccache
4s
Setup CPM Cache
1s
Install Ninja
0s
Setup CLI
1s
Setup Geode SDK
16s
Run CMake and build
1m 5s
Run mkdir -p $HOME/.cache
  -- The CXX compiler identification is Clang 20.1.8 with GNU-like command-line
  -- Detecting CXX compiler ABI info
  -- Detecting CXX compiler ABI info - done
  -- Check for working CXX compiler: C:/Program Files/LLVM/bin/clang++.exe - skipped
  -- Detecting CXX compile features
  -- Detecting CXX compile features - done
  -- Version: 5.11.2
  -- No override path for bindings provided, using CPM to clone default. If you would like to use a separate clone of the bindings repo (for example in order to be able to efficiently change and contribute new bindings) then set GEODE_BINDINGS_REPO_PATH to where you have cloned the repository (system environment variables are supported).
  -- CPM: Adding package bindings@ (main)
  -- Populating bindings
  -- Configuring done (0.1s)
  -- Generating done (0.0s)
  -- Build files have been written to: D:/a/geode-input-buffer/geode-input-buffer/build/_deps/bindings-subbuild
  [1/9] Creating directories for 'bindings-populate'
  [1/9] Performing download step (git clone) for 'bindings-populate'
  Cloning into 'bindings-src'...
  Already on 'main'
  Your branch is up to date with 'origin/main'.
  [2/9] Performing update step for 'bindings-populate'
  -- Fetching latest from the remote origin
  [3/9] No patch step for 'bindings-populate'
  [5/9] No configure step for 'bindings-populate'
  [6/9] No build step for 'bindings-populate'
  [7/9] No install step for 'bindings-populate'
  [8/9] No test step for 'bindings-populate'
  [9/9] Completed 'bindings-populate'
  -- The C compiler identification is Clang 20.1.8 with GNU-like command-line
  -- Detecting C compiler ABI info
  -- Detecting C compiler ABI info - done
  -- Check for working C compiler: C:/Program Files/LLVM/bin/clang.exe - skipped
  -- Detecting C compile features
  -- Detecting C compile features - done
  -- Looking for ccache/sccache
  -- Looking for ccache/sccache - detecting variant
  Using sccache with Clang, PCH will be enabled.
  -- Looking for ccache/sccache - detected sccache
  -- Pre-compiled headers - ON
  -- Found Geode CLI: D:/a/geode-input-buffer/geode-input-buffer/my-epic-geode-cli/geode.exe (version 3.9.0)
  -- CPM: Adding package result@1.4.1 (v1.4.1 to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/result/1b9c)
  -- CPM: Adding package json@3.3.1 (v3.3.1 to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/json/f349)
  -- CPM: Adding package nontype_functional@6 (6d1d08c to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/nontype_functional/aaa0)
  -- CPM: Adding package fmt@5 (5ac44cd to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/fmt/71b2)
  -- {fmt} version: 12.1.0
  -- Build type: Release
  -- Performing Test HAS_NULLPTR_WARNING
  -- Performing Test HAS_NULLPTR_WARNING - Success
  -- CPM: Adding package asp2@ (ecea33b to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/asp2/1cba)
  -- CPM: Adding package arc@1.5.11 (v1.5.11 to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/arc/3609)
  -- Arc features: time
  -- CPM: Adding package TulipHook@3.1.15 (v3.1.15 to D:/a/geode-input-buffer/geode-input-buffer/cpm-cache/tuliphook/e901)
  -- Performing Test SUPPORTS_W_NO_EVERYTHING
  -- Performing Test SUPPORTS_W_NO_EVERYTHING - Success
  Set SKIP_BUILDING_CODEGEN to OFF to force codegen to build locally.
  -- Downloading Codegen
  -- Downloading Codegen - success
  -- Skipping building Codegen
  -- Running Codegen
  'D:/a/geode-input-buffer/geode-input-buffer/build/bindings/codegen/Codegen.exe' 'Win64' 'D:/a/geode-input-buffer/geode-input-buffer/build/_deps/bindings-src/bindings/2.2081' 'D:/a/geode-input-buffer/geode-input-buffer/build/bindings/bindings'
  -- Skipping incompatible Geode-generated source: D:/a/geode-input-buffer/geode-input-buffer/build/_deps/bindings-src/bindings/2.2081/inline/GJBigSpriteNode.cpp
  -- Skipping incompatible Geode-generated source: D:/a/geode-input-buffer/geode-input-buffer/build/_deps/bindings-src/bindings/2.2081/inline/SequenceTriggerGameObject.cpp
  -- Skipping incompatible Geode-generated source: D:/a/geode-input-buffer/geode-input-buffer/build/bindings/bindings/Geode/source/GJBigSpriteNode.cpp
  -- Skipping incompatible Geode-generated source: D:/a/geode-input-buffer/geode-input-buffer/build/bindings/bindings/Geode/source/SequenceTriggerGameObject.cpp
  CMake Error at CMakeLists.txt:43 (message):
    Expected 2 known incompatible Geode binding sources, found 4; do not
    silently build a different SDK layout
  
  
  -- Configuring incomplete, errors occurred!
  Error: Process completed with exit code 1.
