#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "rooksac/fen.hpp"
#include "rooksac/greedy_agent.hpp"
#include "rooksac/match.hpp"
#include "rooksac/negamax_agent.hpp"
#include "rooksac/position.hpp"
#include "rooksac/random_agent.hpp"

namespace {

using namespace rooksac;

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  --white <type>     Agent type for white (default: negamax)\n"
              << "  --black <type>     Agent type for black (default: greedy)\n"
              << "                     Available types: random, greedy, negamax\n"
              << "  --depth <d>        Negamax search depth (default: 2)\n"
              << "  --games <n>        Total number of games to play (default: 1)\n"
              << "  --plies <n>        Max plies per game (default: 150)\n"
              << "  --openings <file>  EPD file with opening FENs (default: startpos only)\n"
              << "  --pgn <file>       Output PGN file to record games\n"
              << "  --swap             Swap colors each game (paired openings)\n";
}

std::unique_ptr<Agent> createAgent(const std::string& type, std::uint32_t depth, std::uint64_t seed) {
    if (type == "random") {
        return std::make_unique<RandomAgent>(seed);
    }
    if (type == "greedy") {
        return std::make_unique<GreedyAgent>();
    }
    if (type == "negamax") {
        return std::make_unique<NegamaxAgent>(depth);
    }
    return nullptr;
}

std::vector<std::string> loadOpenings(const std::string& path) {
    std::vector<std::string> openings;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open openings file " << path << ", using startpos.\n";
        return openings;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // Extract FEN (everything before '|' or full line)
        auto pipePos = line.find('|');
        std::string fen = (pipePos != std::string::npos) ? line.substr(0, pipePos) : line;
        // Trim trailing spaces
        while (!fen.empty() && (fen.back() == ' ' || fen.back() == '\t' || fen.back() == '\r')) {
            fen.pop_back();
        }
        if (!fen.empty()) {
            openings.push_back(fen);
        }
    }
    return openings;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string whiteType = "negamax";
    std::string blackType = "greedy";
    std::uint32_t depth = 2;
    std::uint32_t games = 1;
    std::uint32_t plies = 150;
    std::string openingsFile;
    std::string pgnFile;
    bool swapColors = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        } else if (arg == "--white" && i + 1 < argc) {
            whiteType = argv[++i];
        } else if (arg == "--black" && i + 1 < argc) {
            blackType = argv[++i];
        } else if (arg == "--depth" && i + 1 < argc) {
            depth = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--games" && i + 1 < argc) {
            games = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--plies" && i + 1 < argc) {
            plies = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--openings" && i + 1 < argc) {
            openingsFile = argv[++i];
        } else if (arg == "--pgn" && i + 1 < argc) {
            pgnFile = argv[++i];
        } else if (arg == "--swap") {
            swapColors = true;
        } else if (arg == "--no-swap") {
            swapColors = false;
        } else {
            // Positional fallback for legacy usage: [white] [black] [plies]
            if (i == 1) whiteType = arg;
            else if (i == 2) blackType = arg;
            else if (i == 3) plies = static_cast<std::uint32_t>(std::stoul(arg));
        }
    }

    std::vector<std::string> openings;
    if (!openingsFile.empty()) {
        openings = loadOpenings(openingsFile);
    }
    if (openings.empty()) {
        openings.push_back(std::string(Position::kStartPositionFen));
    }

    std::ofstream pgnStream;
    if (!pgnFile.empty()) {
        pgnStream.open(pgnFile);
        if (!pgnStream.is_open()) {
            std::cerr << "Error: Could not open PGN output file: " << pgnFile << '\n';
            return EXIT_FAILURE;
        }
    }

    std::uint32_t agentAWins = 0;
    std::uint32_t agentBWins = 0;
    std::uint32_t draws = 0;

    std::cout << "Starting match: Agent A (" << whiteType << ") vs Agent B (" << blackType << ")\n"
              << "Games: " << games << ", Max plies: " << plies << ", Search depth: " << depth << '\n'
              << "Openings: " << openings.size() << " positions\n\n";

    for (std::uint32_t g = 0; g < games; ++g) {
        const std::string& openingFen = openings[g % openings.size()];
        auto startPosResult = Position::fromFen(openingFen);
        Position startPos = startPosResult.hasValue() ? startPosResult.value() : Position::startPosition();

        const bool swap = swapColors && (g % 2 == 1);
        const std::string currentWhiteType = swap ? blackType : whiteType;
        const std::string currentBlackType = swap ? whiteType : blackType;

        std::unique_ptr<Agent> white = createAgent(currentWhiteType, depth, 10000 + g * 2);
        std::unique_ptr<Agent> black = createAgent(currentBlackType, depth, 20000 + g * 2 + 1);

        if (!white || !black) {
            std::cerr << "Error creating agents for game " << (g + 1) << '\n';
            return EXIT_FAILURE;
        }

        const MatchResult res = playMatch(*white, *black, startPos, plies);

        // Score accounting from perspective of Agent A
        std::string outcomeStr;
        if (res.winner.has_value()) {
            const bool whiteWon = (*res.winner == Color::White);
            const bool agentAWon = (!swap && whiteWon) || (swap && !whiteWon);
            if (agentAWon) {
                agentAWins++;
                outcomeStr = "Agent A wins";
            } else {
                agentBWins++;
                outcomeStr = "Agent B wins";
            }
        } else {
            draws++;
            outcomeStr = "Draw";
        }

        std::cout << "Game " << (g + 1) << "/" << games << ": "
                  << white->name() << " (W) vs " << black->name() << " (B) -> "
                  << outcomeStr << " (" << res.terminationReason << ", " << res.totalPlies << " plies)\n";

        if (pgnStream.is_open()) {
            std::string pgnGame = res.toPgn(
                white->name(), black->name(),
                "RookSac Benchmark Match", "Local",
                openingFen == Position::kStartPositionFen ? "" : openingFen);
            pgnStream << pgnGame;
        }
    }

    std::cout << "\n================ MATCH RESULTS ================\n"
              << "Agent A (" << whiteType << "): " << agentAWins << " wins\n"
              << "Agent B (" << blackType << "): " << agentBWins << " wins\n"
              << "Draws: " << draws << "\n"
              << "Total: " << games << " games\n"
              << "Score: " << (agentAWins + 0.5 * draws) << "/" << games
              << " (" << (games > 0 ? (agentAWins + 0.5 * draws) / games * 100.0 : 0.0) << "%)\n"
              << "===============================================\n";

    return EXIT_SUCCESS;
}
