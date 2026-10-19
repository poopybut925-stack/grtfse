RESTORE THE REGULAR GEODE BUILD AND COLLECT ROOT-CAUSE EVIDENCE

This is intentionally NOT another workaround. No generated binding .cpp files
are excluded, and no incomplete C++ types are given made-up definitions.

Copy exactly these two files into your existing grtfse GitHub repository:
  CMakeLists.txt
  .github/workflows/build-windows.yml

The contents MUST begin respectively with:
  cmake_minimum_required(VERSION 3.21)
  name: Build Windows geode

Commit. Under Actions, look at the latest build. If it fails, the new step
"Diagnose actual binding definitions if build fails" will print the declarations
needed to repair the underlying Geode binding/type problem.

Send the contents of that step AND the first actual compiler error.

This package does not fix Geode itself yet; it restores the unmodified build
and collects the specific source evidence needed to write a correct fix.
