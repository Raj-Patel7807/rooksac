#include <cassert>
#include <iostream>
#include <random>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"
#include "rooksac/transposition_table.hpp"
#include "rooksac/undo_state.hpp"
#include "rooksac/zobrist.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << '\n';
        std::abort();
    }
}

void testZobristIncrementalVsRecomputed() {
    auto pos = Position::startPosition();
    require(pos.zobristHash() == zobrist::computeHash(pos), "Initial hash matches recomputed hash");

    std::mt19937_64 rng(42);

    // Play a random walk of 100 plies with make/unmake and check incremental equality
    for (int game = 0; game < 10; ++game) {
        Position gamePos = Position::startPosition();
        std::vector<Move> moveHistory;
        std::vector<UndoState> undoHistory;

        for (int ply = 0; ply < 30; ++ply) {
            MoveList legalMoves;
            movegen::generateLegalMoves(gamePos, legalMoves);
            if (legalMoves.empty()) {
                break;
            }

            std::uniform_int_distribution<std::size_t> dist(0, legalMoves.size() - 1);
            const Move m = legalMoves[dist(rng)];

            UndoState undo;
            const bool ok = gamePos.makeMove(m, undo);
            require(ok, "Legal move makes cleanly");

            require(gamePos.zobristHash() == zobrist::computeHash(gamePos),
                    "Incremental hash matches computeHash after makeMove");

            moveHistory.push_back(m);
            undoHistory.push_back(undo);
        }

        // Unmake all moves and check hash restoration
        while (!moveHistory.empty()) {
            const Move m = moveHistory.back();
            const UndoState undo = undoHistory.back();
            moveHistory.pop_back();
            undoHistory.pop_back();

            gamePos.unmakeMove(m, undo);
            require(gamePos.zobristHash() == zobrist::computeHash(gamePos),
                    "Incremental hash matches computeHash after unmakeMove");
        }

        require(gamePos.zobristHash() == pos.zobristHash(), "Startpos hash perfectly restored after all unmakes");
    }
}

void testDistinctKeysForStateDifferences() {
    // 1. Difference in side to move
    auto posWhite = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto posBlack = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    require(posWhite.hasValue() && posBlack.hasValue(), "FENs parsed");
    require(posWhite->zobristHash() != posBlack->zobristHash(), "Flipping side to move alters Zobrist key");

    // 2. Difference in castling rights
    auto posAllCastling = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto posNoCastling = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    require(posAllCastling->zobristHash() != posNoCastling->zobristHash(), "Altering castling rights alters key");

    // 3. Difference in en-passant square
    auto posNoEp = Position::fromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    auto posWithEp = Position::fromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    require(posNoEp->zobristHash() != posWithEp->zobristHash(), "En-passant target square alters key");
}

void testTranspositionTableBoundsAndMateDistance() {
    tt::TranspositionTable table(1024 * 1024);  // 1 MB
    require(table.sizeMb() == 1, "Table sized to 1 MB");
    require(table.permillFull() == 0, "Table is empty initially");

    const std::uint64_t key = 0x123456789ABCDEF0ULL;
    auto moveOpt = Move::create(Square::E2, Square::E4);
    require(moveOpt.has_value(), "Valid move");

    // Store Exact bound with normal score
    table.store(key, /*ply=*/2, /*depth=*/4, /*score=*/150, tt::Bound::Exact, moveOpt);

    auto probeRes = table.probe(key, /*ply=*/2);
    require(probeRes.hit, "TT hit for stored key");
    require(probeRes.score == 150, "Exact score retrieved");
    require(probeRes.depth == 4, "Depth retrieved");
    require(probeRes.bound == tt::Bound::Exact, "Exact bound retrieved");
    require(probeRes.move.has_value() && probeRes.move->encoded() == moveOpt->encoded(), "Move retrieved");

    // Test mate score normalization
    // Suppose from current node at ply 2, checkmate is 3 plies further away (ply 5).
    // The relative score returned at ply 2 is (kMateScore - 5).
    const std::int32_t mateScoreAtPly2 = search::kMateScore - 5;
    const std::uint64_t mateKey = 0xFEEDFACECAFEBEEFULL;
    table.store(mateKey, /*ply=*/2, /*depth=*/5, mateScoreAtPly2, tt::Bound::Exact, moveOpt);

    // Stored score in TT is ply-independent distance to mate from root:
    // scoreToTT(kMateScore - 5, ply=2) = (kMateScore - 5) + 2 = kMateScore - 3.
    // If the exact same position is reached in another branch at ply 4:
    // scoreFromTT(kMateScore - 3, ply=4) = (kMateScore - 3) - 4 = kMateScore - 7.
    // And if probed at the same ply (ply 2):
    auto mateProbeAtPly2 = table.probe(mateKey, /*ply=*/2);
    require(mateProbeAtPly2.hit, "Mate key probe hits at ply 2");
    require(mateProbeAtPly2.score == mateScoreAtPly2, "Same ply retrieves identical mate score");

    auto mateProbeAtPly4 = table.probe(mateKey, /*ply=*/4);
    require(mateProbeAtPly4.hit, "Mate key probe hits at ply 4");
    require(mateProbeAtPly4.score == search::kMateScore - 7, "Different ply adjusts mate distance correctly");

    // Being mated: -kMateScore + 4 at ply 1
    const std::int32_t matedScoreAtPly1 = -search::kMateScore + 4;
    const std::uint64_t matedKey = 0x0102030405060708ULL;
    table.store(matedKey, /*ply=*/1, /*depth=*/3, matedScoreAtPly1, tt::Bound::Exact, moveOpt);

    auto matedProbeAtPly1 = table.probe(matedKey, /*ply=*/1);
    require(matedProbeAtPly1.hit, "Mated key probe hits at ply 1");
    require(matedProbeAtPly1.score == matedScoreAtPly1, "Same ply retrieves identical mated score");

    auto matedProbeAtPly3 = table.probe(matedKey, /*ply=*/3);
    require(matedProbeAtPly3.hit, "Mated key probe hits at ply 3");
    require(matedProbeAtPly3.score == -search::kMateScore + 6, "Deeper ply adjusts mated distance correctly");
}

void testSearchConsistencyWithAndWithoutTT() {
    auto pos = Position::startPosition();
    search::SearchLimits limits(3);

    // 1. Search without TT
    auto resWithout = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateMaterialSideToMove(p);
    });

    // 2. Search with TT
    tt::TranspositionTable ttable(4 * 1024 * 1024);
    auto resWith = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateMaterialSideToMove(p);
    }, nullptr, &ttable);

    require(!resWithout.interrupted && !resWith.interrupted, "Neither search interrupted");
    require(resWithout.bestMove.has_value() && resWith.bestMove.has_value(), "Both return best moves");
    require(resWithout.bestMove->encoded() == resWith.bestMove->encoded(), "TT search yields identical best move");
    require(resWithout.score == resWith.score, "TT search yields identical minimax score");
    require(resWith.stats.ttHits > 0, "Transposition table recorded hits during search");

    std::cout << "TT statistics at depth 3: "
              << resWith.stats.nodes << " nodes, "
              << resWith.stats.ttHits << " TT hits, "
              << resWith.stats.ttCutoffs << " TT cutoffs.\n";
}

}  // namespace

int main() {
    testZobristIncrementalVsRecomputed();
    testDistinctKeysForStateDifferences();
    testTranspositionTableBoundsAndMateDistance();
    testSearchConsistencyWithAndWithoutTT();

    std::cout << "All Zobrist hashing and Transposition Table tests passed successfully.\n";
    return 0;
}
