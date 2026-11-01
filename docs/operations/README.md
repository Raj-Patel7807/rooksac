# Operations

GitHub Actions CI is configured to build and test the native CMake smoke-test target on Linux (GCC and Clang) and Windows (MSVC), and to run formatting, clang-tidy, and Clang ASan/UBSan quality gates. No package, deployment configuration, or hosted environment exists. Future operational assets belong under `deploy/` when they are real, reproducible, and free of credentials or generated environment state.
