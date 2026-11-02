# Benchmarking

RookSac measures performance under strictly reproducible conditions. A number without position set, build configuration, compiler, flags, hardware, and method is not a meaningful comparison.

## Implemented benchmark suite

The benchmark suite is implemented in `rooksac/benchmark.hpp` and `engine/src/benchmark.cpp`, with a standalone CLI runner `rooksac_bench` (`engine/apps/rooksac_bench_main.cpp`) and an integrated UCI `bench` command:

- **Canonical Suite:** 8 diverse positions covering opening (`Startpos`), tactical sharpness (`Kiwipete`, `Tactical-Midgame`), endgames (`Rook-Endgame`, `Queen-Endgame`, `Knight-Pawn`), dynamic pawn structures (`Passed-Pawns`), and kingside attacking play (`Kingside-Attack`).
- **Reproducible Metrics:** Per-position and aggregated total nodes, elapsed time in milliseconds, nodes per second (NPS), best move, and score.
- **Environment Metadata:** Compiler name, version, and build configuration (`Release` vs `Debug`) automatically embedded in reports.
- **Repetitions:** Supports multiple iterations and computes the median node count and elapsed time across runs to eliminate system jitter.
- **Protocol Safety:** In UCI mode, the `bench` command routes diagnostic human-readable reports to `errStream` (stderr) and outputs only a standard single-line `info nodes <N> nps <N>` on stdout, keeping UCI protocol compliance unbroken.

## Usage

```powershell
# Run canonical benchmark with default settings (depth 4)
./rooksac_bench

# Run with custom depth and repetitions
./rooksac_bench --depth 5 --reps 3

# Or via UCI prompt:
bench
```
