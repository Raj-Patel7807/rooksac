# Build and quality checks

RookSac uses CMake 3.23 or newer, C++20, and the Ninja generator. The current build contains one native target, `rooksac_build_smoke_test`, which verifies that the selected compiler accepts C++20 and that CTest is wired correctly. It is build infrastructure, not a chess-engine implementation.

## Prerequisites

- CMake 3.23 or newer.
- Ninja.
- One supported compiler: GCC 12 or newer, Clang 15 or newer, or MSVC from Visual Studio 2022.
- `clang-format` 15 or newer for formatting checks.
- `clang-tidy` 15 or newer only when using the static-analysis preset.

On Windows, run the commands from a Visual Studio Developer PowerShell when using MSVC. The CI workflow constrains Linux tools to GCC 12, Clang 15, and CMake 3.28.3; it does not cache dependencies because the project currently has none.

## Configure, build, and test

From the repository root:

```powershell
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
```

The `release` and `relwithdebinfo` presets use the same commands with their respective names. Preset output is isolated under `out/build/<preset>`, which is ignored by Git.

## Quality targets

```powershell
cmake --build --preset debug --target format-check
cmake --build --preset debug --target format
cmake --preset clang-tidy
cmake --build --preset clang-tidy --parallel
ctest --preset clang-tidy
cmake --preset clang-asan-ubsan
cmake --build --preset clang-asan-ubsan --parallel
ctest --preset clang-asan-ubsan
```

`format-check` fails when a C++ file under `engine/` differs from `.clang-format`; `format` rewrites those files. The `clang-tidy` preset makes the build fail for clang-tidy findings. The `clang-asan-ubsan` preset is intended for Clang on Linux and enables AddressSanitizer and UndefinedBehaviorSanitizer. These presets deliberately target only the project source, not third-party code.

## Current target boundary

No `rooksac_core`, `rooksac_engine`, perft, or benchmark target exists yet because their corresponding engine code does not exist. Future targets must use explicit target dependencies and keep test-only dependencies out of production binaries.
