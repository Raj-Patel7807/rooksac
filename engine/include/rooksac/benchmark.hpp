#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rooksac {
namespace bench {

/// \brief A benchmark test position entry.
struct BenchmarkPosition {
    std::string name;
    std::string fen;
    std::uint32_t depth = 4;
};

/// \brief Statistics from a single benchmark position run.
struct PositionBenchResult {
    std::string name;
    std::uint32_t depth = 0;
    std::uint64_t nodes = 0;
    std::uint64_t timeMs = 0;
    double nps = 0.0;
    std::string bestMove;
    std::int32_t score = 0;
};

/// \brief Aggregated results across a full benchmark run.
struct BenchmarkReport {
    std::vector<PositionBenchResult> results;
    std::uint64_t totalNodes = 0;
    std::uint64_t totalTimeMs = 0;
    double overallNps = 0.0;
    std::string compiler;
    std::string buildType;
};

/// \brief Canonical benchmark test suite of diverse tactical, positional, and endgame positions.
[[nodiscard]] std::vector<BenchmarkPosition> canonicalSuite();

/// \brief Runs the benchmark suite using iterative deepening search with default settings.
/// \param suite The positions to run. If empty, uses canonicalSuite().
/// \param defaultDepth Default search depth for positions that do not specify one.
/// \param repetitions Number of times to repeat the suite (returns median results).
[[nodiscard]] BenchmarkReport runBenchmark(
    const std::vector<BenchmarkPosition>& suite = {},
    std::uint32_t defaultDepth = 4,
    std::uint32_t repetitions = 1);

/// \brief Formats a benchmark report as human-readable text suitable for terminal or log output.
[[nodiscard]] std::string formatReport(const BenchmarkReport& report);

}  // namespace bench
}  // namespace rooksac
