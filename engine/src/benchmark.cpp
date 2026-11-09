#include "rooksac/benchmark.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
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
        {
            "pos01_startpos",
            "Startpos",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            4,
            "Opening development and center control",
            "Standard chess start position"
        },
        // 2. Kiwipete
        {
            "pos02_kiwipete",
            "Kiwipete",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            4,
            "Complex tactical tension, multiple pins, castling options",
            "Peter Jennings Kiwipete / Chess Programming Wiki"
        },
        // 3. Tactical sharp position
        {
            "pos03_tactical_midgame",
            "Tactical-Midgame",
            "r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6",
            4,
            "Pinned knight on c3, tactical counter-attack",
            "Opening trap / middlegame tension"
        },
        // 4. Heavy piece endgame
        {
            "pos04_rook_endgame",
            "Rook-Endgame",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            4,
            "Rook endgame with passed pawns and pin on king",
            "Position 3 from CPW perft suite"
        },
        // 5. Queen vs pieces endgame
        {
            "pos05_queen_endgame",
            "Queen-Endgame",
            "8/5k2/8/8/8/8/1Q6/4K3 w - - 0 1",
            5,
            "King and Queen conversion, mobility restriction",
            "Basic endgame winning technique"
        },
        // 6. Knight and pawn endgame
        {
            "pos06_knight_pawn",
            "Knight-Pawn",
            "8/4k3/8/5N2/8/8/4K3/8 b - - 0 1",
            5,
            "Check evasion and minor piece simplification",
            "Endgame check evasion"
        },
        // 7. Dynamic pawn structure with passed pawns
        {
            "pos07_passed_pawns",
            "Passed-Pawns",
            "8/2p5/1p1p4/pP1P1k2/P1P1pP2/4K3/8/8 w - - 0 1",
            5,
            "Locked pawn structure, breakthrough / zugzwang potential",
            "Endgame pawn block"
        },
        // 8. Open middlegame with kingside attack
        {
            "pos08_kingside_attack",
            "Kingside-Attack",
            "r1b2rk1/1pp1qppp/p1np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R2QR1K1 w - - 0 10",
            4,
            "Middlegame piece coordination and central outpost",
            "Giuoco Pianissimo standard structure"
        }
    };
}

BenchmarkReport runBenchmark(
    const std::vector<BenchmarkPosition>& suite,
    std::uint32_t defaultDepth,
    std::uint32_t repetitions) {
    const auto& positions = suite.empty() ? canonicalSuite() : suite;
    const std::uint32_t reps = std::max(1U, repetitions);

    BenchmarkReport report;
    report.repetitions = reps;

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

#if defined(_WIN32)
    report.os = "Windows (64-bit)";
#elif defined(__linux__)
    report.os = "Linux";
#elif defined(__APPLE__)
    report.os = "macOS";
#else
    report.os = "Unknown OS";
#endif

    report.cpu = "x86_64";
    report.threads = 1;
    report.hashSizeMb = tt::TranspositionTable::kDefaultSizeBytes / (1024 * 1024);

    // Pre-allocate a transposition table
    tt::TranspositionTable ttable(tt::TranspositionTable::kDefaultSizeBytes);

    for (const auto& item : positions) {
        const std::uint32_t targetDepth = (defaultDepth > 0) ? defaultDepth : ((item.depth > 0) ? item.depth : 4);
        auto posOpt = Position::fromFen(item.fen);
        if (!posOpt.hasValue()) {
            continue;
        }

        std::vector<std::uint64_t> repNodes;
        std::vector<std::uint64_t> repQnodes;
        std::vector<std::uint64_t> repTimesMs;
        std::vector<search::SearchStats> repStats;
        std::string bestMoveStr;
        std::string pvStr;
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
            repQnodes.push_back(res.stats.qnodes);
            repTimesMs.push_back(std::max<std::uint64_t>(1ULL, elapsedMs));
            repStats.push_back(res.stats);

            if (r == 0) {
                bestMoveStr = res.bestMove.has_value() ? uci::formatMove(*res.bestMove) : "none";
                pvStr = bestMoveStr;
                finalScore = res.score;
            }
        }

        // Compute median across repetitions
        std::vector<std::size_t> indices(reps);
        for (std::size_t i = 0; i < reps; ++i) {
            indices[i] = i;
        }
        std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
            return repTimesMs[a] < repTimesMs[b];
        });

        const std::size_t midIdx = indices[reps / 2];
        const std::uint64_t medianNodes = repNodes[midIdx];
        const std::uint64_t medianQnodes = repQnodes[midIdx];
        const std::uint64_t medianTimeMs = repTimesMs[midIdx];
        const auto& chosenStats = repStats[midIdx];

        // Compute sample standard deviation for execution time
        double dispersion = 0.0;
        if (reps > 1) {
            double sum = 0.0;
            for (std::uint64_t t : repTimesMs) {
                sum += static_cast<double>(t);
            }
            const double mean = sum / static_cast<double>(reps);
            double sqDiffSum = 0.0;
            for (std::uint64_t t : repTimesMs) {
                const double diff = static_cast<double>(t) - mean;
                sqDiffSum += diff * diff;
            }
            dispersion = std::sqrt(sqDiffSum / static_cast<double>(reps - 1));
        }

        const double nps = (static_cast<double>(medianNodes) * 1000.0) /
                           static_cast<double>(medianTimeMs);

        PositionBenchResult posResult;
        posResult.id = item.id;
        posResult.name = item.name;
        posResult.depth = targetDepth;
        posResult.nodes = medianNodes;
        posResult.qnodes = medianQnodes;
        posResult.timeMs = medianTimeMs;
        posResult.nps = nps;
        posResult.bestMove = bestMoveStr;
        posResult.pv = pvStr;
        posResult.score = finalScore;
        posResult.cutoffs = chosenStats.cutoffs;
        posResult.ttProbes = chosenStats.ttProbes;
        posResult.ttHits = chosenStats.ttHits;
        posResult.ttCutoffs = chosenStats.ttCutoffs;
        posResult.nmpCutoffs = chosenStats.nmpCutoffs;
        posResult.lmrReductions = chosenStats.lmrReductions;
        posResult.lmrResearches = chosenStats.lmrResearches;
        posResult.timeDispersionMs = dispersion;

        report.results.push_back(posResult);
        report.totalNodes += medianNodes;
        report.totalQnodes += medianQnodes;
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
    ss << "=========================================================================================\n";
    ss << " RookSac Benchmark Suite Report\n";
    ss << " Compiler:    " << report.compiler << " (" << report.buildType << ")\n";
    ss << " OS / CPU:    " << report.os << " / " << report.cpu << "\n";
    ss << " Hash / Th:   " << report.hashSizeMb << " MB / " << report.threads << " thread(s)\n";
    ss << " Repetitions: " << report.repetitions << " (reporting median and dispersion)\n";
    ss << " Positions:   " << report.results.size() << "\n";
    ss << "=========================================================================================\n";
    ss << std::left << std::setw(18) << "Position"
       << std::right << std::setw(5) << "D"
       << std::setw(10) << "Nodes"
       << std::setw(9) << "QNodes"
       << std::setw(8) << "Time"
       << std::setw(6) << "±s"
       << std::setw(12) << "NPS"
       << std::setw(7) << "TT-Hit"
       << std::setw(6) << "Cutoff"
       << std::setw(8) << "Move"
       << std::setw(7) << "Score"
       << "\n";
    ss << "-----------------------------------------------------------------------------------------\n";

    for (const auto& res : report.results) {
        ss << std::left << std::setw(18) << res.name
           << std::right << std::setw(5) << res.depth
           << std::setw(10) << res.nodes
           << std::setw(9) << res.qnodes
           << std::setw(8) << res.timeMs
           << std::setw(6) << std::fixed << std::setprecision(1) << res.timeDispersionMs
           << std::setw(12) << static_cast<std::uint64_t>(res.nps)
           << std::setw(7) << res.ttHits
           << std::setw(6) << res.cutoffs
           << std::setw(8) << res.bestMove
           << std::setw(7) << res.score
           << "\n";
    }

    ss << "-----------------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(18) << "Total / Overall"
       << std::right << std::setw(5) << "-"
       << std::setw(10) << report.totalNodes
       << std::setw(9) << report.totalQnodes
       << std::setw(8) << report.totalTimeMs
       << std::setw(6) << "-"
       << std::setw(12) << static_cast<std::uint64_t>(report.overallNps)
       << std::setw(7) << "-"
       << std::setw(6) << "-"
       << std::setw(8) << "-"
       << std::setw(7) << "-"
       << "\n";
    ss << "=========================================================================================\n";

    return ss.str();
}

std::string formatJson(const BenchmarkReport& report) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"compiler\": \"" << report.compiler << "\",\n";
    ss << "  \"build_type\": \"" << report.buildType << "\",\n";
    ss << "  \"os\": \"" << report.os << "\",\n";
    ss << "  \"cpu\": \"" << report.cpu << "\",\n";
    ss << "  \"hash_size_mb\": " << report.hashSizeMb << ",\n";
    ss << "  \"threads\": " << report.threads << ",\n";
    ss << "  \"repetitions\": " << report.repetitions << ",\n";
    ss << "  \"total_nodes\": " << report.totalNodes << ",\n";
    ss << "  \"total_qnodes\": " << report.totalQnodes << ",\n";
    ss << "  \"total_time_ms\": " << report.totalTimeMs << ",\n";
    ss << "  \"overall_nps\": " << static_cast<std::uint64_t>(report.overallNps) << ",\n";
    ss << "  \"positions\": [\n";

    for (std::size_t i = 0; i < report.results.size(); ++i) {
        const auto& r = report.results[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << r.id << "\",\n";
        ss << "      \"name\": \"" << r.name << "\",\n";
        ss << "      \"depth\": " << r.depth << ",\n";
        ss << "      \"nodes\": " << r.nodes << ",\n";
        ss << "      \"qnodes\": " << r.qnodes << ",\n";
        ss << "      \"time_ms\": " << r.timeMs << ",\n";
        ss << "      \"time_dispersion_ms\": " << std::fixed << std::setprecision(2) << r.timeDispersionMs << ",\n";
        ss << "      \"nps\": " << static_cast<std::uint64_t>(r.nps) << ",\n";
        ss << "      \"best_move\": \"" << r.bestMove << "\",\n";
        ss << "      \"pv\": \"" << r.pv << "\",\n";
        ss << "      \"score\": " << r.score << ",\n";
        ss << "      \"cutoffs\": " << r.cutoffs << ",\n";
        ss << "      \"tt_probes\": " << r.ttProbes << ",\n";
        ss << "      \"tt_hits\": " << r.ttHits << ",\n";
        ss << "      \"tt_cutoffs\": " << r.ttCutoffs << ",\n";
        ss << "      \"nmp_cutoffs\": " << r.nmpCutoffs << ",\n";
        ss << "      \"lmr_reductions\": " << r.lmrReductions << ",\n";
        ss << "      \"lmr_researches\": " << r.lmrResearches << "\n";
        ss << "    }" << (i + 1 < report.results.size() ? "," : "") << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

std::string formatCsv(const BenchmarkReport& report) {
    std::ostringstream ss;
    ss << "id,name,depth,nodes,qnodes,time_ms,time_dispersion_ms,nps,best_move,pv,score,cutoffs,tt_probes,tt_hits,tt_cutoffs,nmp_cutoffs,lmr_reductions,lmr_researches\n";
    for (const auto& r : report.results) {
        ss << r.id << ","
           << r.name << ","
           << r.depth << ","
           << r.nodes << ","
           << r.qnodes << ","
           << r.timeMs << ","
           << std::fixed << std::setprecision(2) << r.timeDispersionMs << ","
           << static_cast<std::uint64_t>(r.nps) << ","
           << r.bestMove << ","
           << r.pv << ","
           << r.score << ","
           << r.cutoffs << ","
           << r.ttProbes << ","
           << r.ttHits << ","
           << r.ttCutoffs << ","
           << r.nmpCutoffs << ","
           << r.lmrReductions << ","
           << r.lmrResearches << "\n";
    }
    return ss.str();
}

}  // namespace bench
}  // namespace rooksac
