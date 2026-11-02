#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "rooksac/greedy_agent.hpp"
#include "rooksac/match.hpp"
#include "rooksac/negamax_agent.hpp"
#include "rooksac/position.hpp"
#include "rooksac/random_agent.hpp"

namespace {

using namespace rooksac;

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [random|greedy|negamax] [random|greedy|negamax] [plies]\n"
              << "Example: " << prog << " negamax greedy 100\n";
}

std::unique_ptr<Agent> createAgent(const std::string& type, std::uint64_t seed) {
    if (type == "random") {
        return std::make_unique<RandomAgent>(seed);
    }
    if (type == "greedy") {
        return std::make_unique<GreedyAgent>();
    }
    if (type == "negamax") {
        return std::make_unique<NegamaxAgent>(/*depth=*/2);
    }
    return nullptr;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string whiteType = "greedy";
    std::string blackType = "random";
    std::uint32_t plies = 100;

    if (argc >= 2) whiteType = argv[1];
    if (argc >= 3) blackType = argv[2];
    if (argc >= 4) plies = static_cast<std::uint32_t>(std::stoul(argv[3]));

    std::unique_ptr<Agent> white = createAgent(whiteType, 12345);
    if (!white) {
        std::cerr << "Unknown white agent type: " << whiteType << '\n';
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    std::unique_ptr<Agent> black = createAgent(blackType, 67890);
    if (!black) {
        std::cerr << "Unknown black agent type: " << blackType << '\n';
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    std::cout << "Starting match: " << white->name() << " (White) vs " << black->name() << " (Black)\n";
    const MatchResult result = playMatch(*white, *black, Position::startPosition(), plies);

    std::cout << "Match concluded in " << result.totalPlies << " plies.\n";
    std::cout << "Termination reason: " << result.terminationReason << '\n';
    if (result.winner.has_value()) {
        std::cout << "Winner: " << (*result.winner == Color::White ? "White" : "Black") << '\n';
    } else {
        std::cout << "Result: Draw\n";
    }
    std::cout << "Final FEN: " << result.finalFen << '\n';

    return EXIT_SUCCESS;
}
