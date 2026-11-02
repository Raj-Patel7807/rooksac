#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "rooksac/greedy_agent.hpp"
#include "rooksac/match.hpp"
#include "rooksac/position.hpp"
#include "rooksac/random_agent.hpp"

namespace {

using namespace rooksac;

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [random|greedy] [random|greedy] [plies]\n"
              << "Example: " << prog << " greedy random 100\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string whiteType = "greedy";
    std::string blackType = "random";
    std::uint32_t plies = 100;

    if (argc >= 2) whiteType = argv[1];
    if (argc >= 3) blackType = argv[2];
    if (argc >= 4) plies = static_cast<std::uint32_t>(std::stoul(argv[3]));

    std::unique_ptr<Agent> white;
    std::unique_ptr<Agent> black;

    if (whiteType == "random") {
        white = std::make_unique<RandomAgent>(12345);
    } else if (whiteType == "greedy") {
        white = std::make_unique<GreedyAgent>();
    } else {
        std::cerr << "Unknown white agent type: " << whiteType << '\n';
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (blackType == "random") {
        black = std::make_unique<RandomAgent>(67890);
    } else if (blackType == "greedy") {
        black = std::make_unique<GreedyAgent>();
    } else {
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
