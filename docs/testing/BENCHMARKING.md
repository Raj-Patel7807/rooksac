# Benchmarking

RookSac will measure performance only with reproducible conditions. A number without position set, build configuration, compiler, flags, hardware, and method is not a meaningful comparison.

## Planned measurements

- Nodes per second (NPS), with node definition stated.
- Reached search depth and elapsed search time.
- Memory consumption, including transposition-table configuration.
- Fixed benchmark positions and position-set version.
- Before/after results from the same environment.

## Method

Use release-oriented builds only after a real build exists, warm up when appropriate, repeat runs, record variance or raw runs, and distinguish throughput from playing strength. Pin or disclose relevant OS, CPU, compiler, CMake generator, flags, and clock source. Avoid comparing results across different machines as if they were direct optimization evidence.

No benchmark harness or results exist in Phase 0. See [Performance](PERFORMANCE.md).
