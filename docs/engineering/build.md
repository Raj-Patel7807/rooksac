# Build policy

RookSac does not yet have a CMake project or a buildable target. Therefore this document intentionally provides no build command.

## Intended direction

CMake will define the project’s C++20 targets, compiler feature requirements, warning policy, test integration, and install or packaging choices when those become real needs. Initial platform support targets Windows and Linux; portability decisions should avoid platform-specific assumptions in engine code.

## Configurations

Future development will use separate Debug and Release configurations. Debug builds should favor diagnostics and assertions; Release builds should be used for performance measurements only after recording compiler, flags, hardware, and benchmark inputs.

## Future validation

When a build system exists, its documentation must state exact prerequisite versions and commands that have been run. Test integration, sanitizers, formatting, and CI should be introduced with real targets rather than speculative configuration.
