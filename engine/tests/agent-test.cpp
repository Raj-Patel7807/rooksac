#include <cstdlib>
#include <iostream>
#include <string>

#include "rooksac/greedy_agent.hpp"
#include "rooksac/legal_movegen.hpp"
#include "rooksac/match.hpp"
#include "rooksac/material.hpp"
#include "rooksac/position.hpp"
#include "rooksac/random_agent.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testMaterialEvaluation() {
    const auto startPos = Position::startPosition();
    require(eval::evaluateMaterial(startPos) == 0, "Start position material balance is 0");
    require(eval::evaluateMaterialSideToMove(startPos) == 0, "Start position material balance for side to move is 0");

    // Position with White up a queen: 8/8/8/8/8/8/8/k3Q2K w - - 0 1
    auto upQueenOpt = Position::fromFen("8/8/8/8/8/8/8/k3Q2K w - - 0 1");
    require(upQueenOpt.hasValue(), "FEN up a queen parses");
    require(eval::evaluateMaterial(*upQueenOpt) == 900, "White is +900 centipawns with Queen");
    require(eval::evaluateMaterialSideToMove(*upQueenOpt) == 900, "Side to move White has +900");

    // Change side to move to Black
    auto blackTurnOpt = Position::fromFen("8/8/8/8/8/8/8/k3Q2K b - - 0 1");
    require(blackTurnOpt.hasValue(), "FEN black turn parses");
    require(eval::evaluateMaterial(*blackTurnOpt) == 900, "White perspective is still +900");
    require(eval::evaluateMaterialSideToMove(*blackTurnOpt) == -900, "Side to move Black is -900");

    // Custom material scale
    eval::MaterialWeights customWeights;
    customWeights.queen = 1000;
    require(eval::evaluateMaterial(*upQueenOpt, customWeights) == 1000, "Custom weight evaluation respected");
}

void testRandomAgent() {
    auto pos = Position::startPosition();
    const std::string originalFen = pos.toFen();

    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    RandomAgent agent1(12345);
    const auto m1 = agent1.selectMove(pos, legalMoves);
    require(m1.has_value(), "Random agent returns a move");
    require(legalMoves.contains(*m1), "Random move must be strictly legal");
    require(pos.toFen() == originalFen, "Position state must not be modified after selection");

    // Seed reproducibility
    RandomAgent agent2(12345);
    const auto m2 = agent2.selectMove(pos, legalMoves);
    require(m2.has_value() && m1->encoded() == m2->encoded(), "Identical seed produces identical move");

    // Empty legal moves (terminal state)
    auto staleOpt = Position::fromFen("k7/2Q5/2K5/8/8/8/8/8 b - - 0 1");
    auto stalePos = *staleOpt;
    MoveList emptyMoves;
    movegen::generateLegalMoves(stalePos, emptyMoves);
    require(!agent1.selectMove(stalePos, emptyMoves).has_value(), "Agent returns nullopt on terminal position");
}

void testGreedyAgentFreeCapture() {
    // White queen can capture an undefended black queen on e5:
    // FEN: 4k3/8/8/4q3/4Q3/8/8/4K3 w - - 0 1
    // Legal queen moves include taking on e5 (score jumps from 0 to +900)
    auto posOpt = Position::fromFen("4k3/8/8/4q3/4Q3/8/8/4K3 w - - 0 1");
    require(posOpt.hasValue(), "Free queen FEN parses");
    auto pos = *posOpt;
    const std::string originalFen = pos.toFen();

    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    GreedyAgent greedy;
    const auto chosenMove = greedy.selectMove(pos, legalMoves);
    require(chosenMove.has_value(), "Greedy agent chooses a move");
    require(chosenMove->from() == Square::E4 && chosenMove->to() == Square::E5,
            "Greedy agent captures free queen on e5");
    require(pos.toFen() == originalFen, "Position state preserved after greedy selection");
}

void testGreedyAgentDefendedPieceWeakness() {
    // Documented weakness test:
    // White queen on d1 can capture a defended pawn on d5 (defended by black pawn on c6).
    // FEN: 4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1
    // Greedy evaluates solely based on the resulting material (+100 for taking d5)
    // and cannot see that black will play cxd5 capturing the queen next turn!
    auto posOpt = Position::fromFen("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1");
    require(posOpt.hasValue(), "Defended pawn FEN parses");
    auto pos = *posOpt;

    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    GreedyAgent greedy;
    const auto chosenMove = greedy.selectMove(pos, legalMoves);
    require(chosenMove.has_value(), "Greedy agent chooses a move");
    require(chosenMove->from() == Square::D1 && chosenMove->to() == Square::D5,
            "Demonstrates greedy weakness: captures defended pawn with queen for immediate +100");
}

void testSimulatedMatches() {
    // 1. Random vs Random game terminates cleanly
    RandomAgent whiteRand(42);
    RandomAgent blackRand(99);
    const MatchResult matchRand = playMatch(whiteRand, blackRand, Position::startPosition(), /*maxPlies=*/100);
    require(matchRand.totalPlies > 0, "Random game played moves");
    require(matchRand.result != GameResult::Ongoing || matchRand.totalPlies == 100,
            "Random game concluded or hit max plies");

    // 2. Greedy vs Random match
    GreedyAgent whiteGreedy;
    RandomAgent blackRand2(777);
    const MatchResult matchGreedy = playMatch(whiteGreedy, blackRand2, Position::startPosition(), /*maxPlies=*/100);
    require(matchGreedy.totalPlies > 0, "Greedy vs Random game played moves");

    // In a position where White is already ahead in material, Greedy reliably converts or captures
    auto aheadOpt = Position::fromFen("4k3/8/8/8/8/8/8/4K2Q w - - 0 1");
    auto aheadPos = *aheadOpt;
    const MatchResult aheadMatch = playMatch(whiteGreedy, blackRand2, aheadPos, /*maxPlies=*/60);
    require(aheadMatch.totalPlies > 0, "Ahead match played moves");
}

}  // namespace

int main() {
    testMaterialEvaluation();
    testRandomAgent();
    testGreedyAgentFreeCapture();
    testGreedyAgentDefendedPieceWeakness();
    testSimulatedMatches();

    std::cout << "All Random and Greedy baseline agent tests passed successfully.\n";
    return EXIT_SUCCESS;
}

