$ErrorActionPreference = 'Stop'
Write-Host "Building Predictive Safe Click v0.3 for Geometry Dash 2.2081"
if (-not (Get-Command geode -ErrorAction SilentlyContinue)) {
    throw "Geode CLI missing. Install it: winget install GeodeSDK.GeodeCLI"
}
if (-not $env:GEODE_SDK) {
    Write-Host "Downloading Geode SDK..."
    geode sdk install
    throw "Restart PowerShell after the SDK is installed and rerun this script."
}
if (-not (Test-Path "$env:GEODE_SDK/CMakeLists.txt")) {
    throw "GEODE_SDK is invalid: $env:GEODE_SDK"
}
Write-Host "Configuring with CMake..."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed. Copy its error into chat.' }
cmake --build build --config Release --target PredictiveSafeClick_PACKAGE
if ($LASTEXITCODE -ne 0) { throw 'Compile failed. Copy the last 30 error lines into chat.' }
Write-Host "Looking for .geode output:"
Get-ChildItem -Path build -Filter '*.geode' -Recurse | Select-Object FullName
