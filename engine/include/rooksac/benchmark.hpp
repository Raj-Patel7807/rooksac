#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rooksac {
namespace bench {

/// \brief A benchmark test position entry with metadata.
struct BenchmarkPosition {
    std::string id;          ///< Unique stable identifier (e.g., "pos01_startpos")
    std::string name;        ///< Human-readable name
    std::string fen;         ///< Full FEN string
    std::uint32_t depth = 4; ///< Recommended test depth
    std::string theme;       ///< Tactical / strategic theme (e.g., "Opening development", "Piece pin / horizon")
    std::string provenance;  ///< Origin / source (e.g., "Standard start position", "Peter Jennings Kiwipete / CPW")
};

/// \brief Detailed metrics from a single benchmark position run.
struct PositionBenchResult {
    std::string id;
    std::string name;
    std::uint32_t depth = 0;
    std::uint64_t nodes = 0;
    std::uint64_t qnodes = 0;
    std::uint64_t timeMs = 0;
    double nps = 0.0;
    std::string bestMove;
    std::string pv;
    std::int32_t score = 0;
    std::uint64_t cutoffs = 0;
    std::uint64_t ttProbes = 0;
    std::uint64_t ttHits = 0;
    std::uint64_t ttCutoffs = 0;
    std::uint64_t nmpCutoffs = 0;
    std::uint64_t lmrReductions = 0;
    std::uint64_t lmrResearches = 0;
    double timeDispersionMs = 0.0; ///< Sample standard deviation across repetitions
};

/// \brief Aggregated results across a full benchmark run.
struct BenchmarkReport {
    std::vector<PositionBenchResult> results;
    std::uint64_t totalNodes = 0;
    std::uint64_t totalQnodes = 0;
    std::uint64_t totalTimeMs = 0;
    double overallNps = 0.0;
    std::string compiler;
    std::string buildType;
    std::string cpu;
    std::string os;
    std::size_t hashSizeMb = 16;
    std::uint32_t threads = 1;
    std::uint32_t repetitions = 1;
};

/// \brief Canonical benchmark test suite of diverse tactical, positional, and endgame positions.
[[nodiscard]] std::vector<BenchmarkPosition> canonicalSuite();

/// \brief Runs the benchmark suite using iterative deepening search with default settings.
/// \param suite The positions to run. If empty, uses canonicalSuite().
/// \param defaultDepth Default search depth for positions that do not specify one.
/// \param repetitions Number of times to repeat the suite (computes median and dispersion).
[[nodiscard]] BenchmarkReport runBenchmark(
    const std::vector<BenchmarkPosition>& suite = {},
    std::uint32_t defaultDepth = 4,
    std::uint32_t repetitions = 1);

/// \brief Formats a benchmark report as human-readable text suitable for terminal or log output.
[[nodiscard]] std::string formatReport(const BenchmarkReport& report);

/// \brief Formats a benchmark report as structured JSON.
[[nodiscard]] std::string formatJson(const BenchmarkReport& report);

/// \brief Formats a benchmark report as CSV.
[[nodiscard]] std::string formatCsv(const BenchmarkReport& report);

}  // namespace bench
}  // namespace rooksac
