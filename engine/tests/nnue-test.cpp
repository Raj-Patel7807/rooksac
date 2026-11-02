#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/nnue.hpp"
#include "rooksac/position.hpp"

namespace {

using namespace rooksac;
using namespace rooksac::nnue;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testAccumulatorIncrementalUpdateParity() {
    auto posOpt = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    require(posOpt.hasValue(), "Startpos parses");
    auto pos = *posOpt;

    NnueEvaluator evaluator;
    evaluator.initialize();

    // 1. Initial full accumulator
    Accumulator accum = evaluator.computeAccumulator(pos);

    // 2. Play 1. e2-e4
    auto moveOpt = Move::create(Square::E2, Square::E4);
    require(moveOpt.has_value(), "Move creates");

    std::vector<std::pair<Piece, Square>> removed = {{Piece::WhitePawn, Square::E2}};
    std::vector<std::pair<Piece, Square>> added = {{Piece::WhitePawn, Square::E4}};

    // Incremental update
    evaluator.updateAccumulator(accum, added, removed);

    UndoState undo;
    (void)pos.makeMove(*moveOpt, undo);

    // Full recomputation on new position
    const Accumulator recomputed = evaluator.computeAccumulator(pos);

    // 3. Parity check: incremental update must match full recomputation across all hidden units
    for (std::size_t h = 0; h < kHiddenSize; ++h) {
        require(accum.values[h] == recomputed.values[h], "Incremental accumulator matches full computation");
    }

    // 4. Parity check on unmake: revert incremental changes
    evaluator.updateAccumulator(accum, removed, added); // Swap added/removed to undo
    pos.unmakeMove(*moveOpt, undo);

    const Accumulator startAccum = evaluator.computeAccumulator(pos);
    for (std::size_t h = 0; h < kHiddenSize; ++h) {
        require(accum.values[h] == startAccum.values[h], "Unmade accumulator matches start position");
    }
}

void testNnueColourSymmetry() {
    auto whiteOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    auto blackOpt = Position::fromFen("3qk3/8/8/8/8/8/5PPP/7K b - - 0 1");
    require(whiteOpt.hasValue() && blackOpt.hasValue(), "Positions parse");

    NnueEvaluator evaluator;
    evaluator.initialize();

    const std::int32_t whiteScore = evaluator.evaluate(*whiteOpt);
    const std::int32_t blackScore = evaluator.evaluate(*blackOpt);

    require(whiteScore == blackScore, "NNUE scores are symmetric from side-to-move perspective");
}

}  // namespace

int main() {
    testAccumulatorIncrementalUpdateParity();
    testNnueColourSymmetry();

    std::cout << "All NNUE research track tests passed successfully.\n";
    return EXIT_SUCCESS;
}
