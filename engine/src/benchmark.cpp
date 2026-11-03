#include "rooksac/benchmark.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "rooksac/position.hpp"
#include "rooksac/search.hpp"
#include "rooksac/transposition_table.hpp"
#include "rooksac/uci.hpp"

namespace rooksac {
namespace bench {

std::vector<BenchmarkPosition> canonicalSuite() {
    return {
        // 1. Initial starting position
        {"Startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4},
        // 2. Kiwipete (complex tactical position with multiple pieces under tension)
        {"Kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
        // 3. Tactical sharp position (Queen and minor piece tactics)
        {"Tactical-Midgame", "r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6", 4},
        // 4. Heavy piece endgame (Rook and pawn endgame)
        {"Rook-Endgame", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4},
        // 5. Queen vs pieces endgame
        {"Queen-Endgame", "8/5k2/8/8/8/8/1Q6/4K3 w - - 0 1", 5},
        // 6. Knight and pawn endgame
        {"Knight-Pawn", "8/4k3/8/5N2/8/8/4K3/8 b - - 0 1", 5},
        // 7. Dynamic pawn structure with passed pawns
        {"Passed-Pawns", "8/2p5/1p1p4/pP1P1k2/P1P1pP2/4K3/8/8 w - - 0 1", 5},
        // 8. Open middlegame with kingside attack
        {"Kingside-Attack", "r1b2rk1/1pp1qppp/p1np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R2QR1K1 w - - 0 10", 4}
    };
}

BenchmarkReport runBenchmark(
    const std::vector<BenchmarkPosition>& suite,
    std::uint32_t defaultDepth,
    std::uint32_t repetitions) {
    const auto& positions = suite.empty() ? canonicalSuite() : suite;
    const std::uint32_t reps = std::max(1U, repetitions);

    BenchmarkReport report;

#if defined(__clang__)
    report.compiler = "Clang " + std::to_string(__clang_major__) + "." +
                      std::to_string(__clang_minor__) + "." +
                      std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
    report.compiler = "GCC " + std::to_string(__GNUC__) + "." +
                      std::to_string(__GNUC_MINOR__) + "." +
                      std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    report.compiler = "MSVC " + std::to_string(_MSC_VER);
#else
    report.compiler = "Unknown C++20 Compiler";
#endif

#if defined(NDEBUG)
    report.buildType = "Release";
#else
    report.buildType = "Debug";
#endif

    // Pre-allocate a transposition table using the default size for benchmark runs
    tt::TranspositionTable ttable(tt::TranspositionTable::kDefaultSizeBytes);

    for (const auto& item : positions) {
        const std::uint32_t targetDepth = (item.depth > 0) ? item.depth : defaultDepth;
        auto posOpt = Position::fromFen(item.fen);
        if (!posOpt.hasValue()) {
            continue;
        }

        std::vector<std::uint64_t> repNodes;
        std::vector<std::uint64_t> repTimesMs;
        std::string bestMoveStr;
        std::int32_t finalScore = 0;

        for (std::uint32_t r = 0; r < reps; ++r) {
            ttable.clear();
            Position pos = *posOpt;

            search::SearchLimits limits(targetDepth);
            const auto startTime = std::chrono::steady_clock::now();
            const auto res = search::search(pos, limits, [](const Position& p) {
                return eval::evaluateSideToMove(p);
            }, nullptr, &ttable);
            const auto endTime = std::chrono::steady_clock::now();

            const auto elapsedMs = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

            repNodes.push_back(res.stats.nodes);
            repTimesMs.push_back(std::max<std::uint64_t>(1ULL, elapsedMs));

            if (r == 0) {
                bestMoveStr = res.bestMove.has_value() ? uci::formatMove(*res.bestMove) : "none";
                finalScore = res.score;
            }
        }

        // Use median across repetitions
        std::sort(repNodes.begin(), repNodes.end());
        std::sort(repTimesMs.begin(), repTimesMs.end());
        const std::size_t midIdx = reps / 2;
        const std::uint64_t medianNodes = repNodes[midIdx];
        const std::uint64_t medianTimeMs = repTimesMs[midIdx];
        const double nps = (static_cast<double>(medianNodes) * 1000.0) /
                           static_cast<double>(medianTimeMs);

        PositionBenchResult posResult;
        posResult.name = item.name;
        posResult.depth = targetDepth;
        posResult.nodes = medianNodes;
        posResult.timeMs = medianTimeMs;
        posResult.nps = nps;
        posResult.bestMove = bestMoveStr;
        posResult.score = finalScore;

        report.results.push_back(posResult);
        report.totalNodes += medianNodes;
        report.totalTimeMs += medianTimeMs;
    }

    if (report.totalTimeMs > 0) {
        report.overallNps = (static_cast<double>(report.totalNodes) * 1000.0) /
                            static_cast<double>(report.totalTimeMs);
    }

    return report;
}

std::string formatReport(const BenchmarkReport& report) {
    std::ostringstream ss;
    ss << "================================================================================\n";
    ss << " RookSac Benchmark Suite Report\n";
    ss << " Compiler:   " << report.compiler << " (" << report.buildType << ")\n";
    ss << " Positions:  " << report.results.size() << "\n";
    ss << "================================================================================\n";
    ss << std::left << std::setw(18) << "Position"
       << std::right << std::setw(6) << "Depth"
       << std::setw(12) << "Nodes"
       << std::setw(10) << "Time (ms)"
       << std::setw(14) << "NPS"
       << std::setw(10) << "BestMove"
       << std::setw(10) << "Score"
       << "\n";
    ss << "--------------------------------------------------------------------------------\n";

    for (const auto& res : report.results) {
        ss << std::left << std::setw(18) << res.name
           << std::right << std::setw(6) << res.depth
           << std::setw(12) << res.nodes
           << std::setw(10) << res.timeMs
           << std::setw(14) << static_cast<std::uint64_t>(res.nps)
           << std::setw(10) << res.bestMove
           << std::setw(10) << res.score
           << "\n";
    }

    ss << "--------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(18) << "Total / Overall"
       << std::right << std::setw(6) << "-"
       << std::setw(12) << report.totalNodes
       << std::setw(10) << report.totalTimeMs
       << std::setw(14) << static_cast<std::uint64_t>(report.overallNps)
       << "\n";
    ss << "================================================================================\n";

    return ss.str();
}

}  // namespace bench
}  // namespace rooksac
