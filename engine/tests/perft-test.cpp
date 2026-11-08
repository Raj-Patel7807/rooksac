#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/perft.hpp"
#include "rooksac/position.hpp"
#include "rooksac/zobrist.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testStartPositionPerft() {
    auto pos = Position::startPosition();

    // Standard initial position counts (source: Chess Programming Wiki)
    // Depth 1: 20
    // Depth 2: 400
    // Depth 3: 8,902
    // Depth 4: 197,281
    require(perft::perft(pos, 1) == 20ULL, "Start pos perft(1) == 20");
    require(perft::perft(pos, 2) == 400ULL, "Start pos perft(2) == 400");
    require(perft::perft(pos, 3) == 8902ULL, "Start pos perft(3) == 8,902");
    require(perft::perft(pos, 4) == 197281ULL, "Start pos perft(4) == 197,281");

    // Test divide consistency
    const auto divResult = perft::divide(pos, 2);
    require(divResult.totalNodes == 400ULL, "Divide totalNodes matches perft");
    require(divResult.divideEntries.size() == 20, "Divide entry count matches legal moves");
}

void testKiwipetePerft() {
    // Position 2: Kiwipete
    // r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
    // Depth 1: 48
    // Depth 2: 2,039
    // Depth 3: 97,862
    // Depth 4: 4,085,603
    auto posOpt = Position::fromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    require(posOpt.hasValue(), "Kiwipete FEN parses");
    auto pos = *posOpt;

    require(perft::perft(pos, 1) == 48ULL, "Kiwipete perft(1) == 48");
    require(perft::perft(pos, 2) == 2039ULL, "Kiwipete perft(2) == 2,039");
    require(perft::perft(pos, 3) == 97862ULL, "Kiwipete perft(3) == 97,862");
}

void testPosition3Perft() {
    // Position 3 (CPW: 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1)
    // Depth 1: 14
    // Depth 2: 191
    // Depth 3: 2,812
    // Depth 4: 43,238
    auto posOpt = Position::fromFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    require(posOpt.hasValue(), "Position 3 FEN parses");
    auto pos = *posOpt;

    require(perft::perft(pos, 1) == 14ULL, "Position 3 perft(1) == 14");
    require(perft::perft(pos, 2) == 191ULL, "Position 3 perft(2) == 191");
    require(perft::perft(pos, 3) == 2812ULL, "Position 3 perft(3) == 2,812");
    require(perft::perft(pos, 4) == 43238ULL, "Position 3 perft(4) == 43,238");
}

void testPosition4Perft() {
    // Position 4 (CPW: r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1)
    // Depth 1: 6
    // Depth 2: 264
    // Depth 3: 9,467
    auto posOpt = Position::fromFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    require(posOpt.hasValue(), "Position 4 FEN parses");
    auto pos = *posOpt;

    require(perft::perft(pos, 1) == 6ULL, "Position 4 perft(1) == 6");
    require(perft::perft(pos, 2) == 264ULL, "Position 4 perft(2) == 264");
    require(perft::perft(pos, 3) == 9467ULL, "Position 4 perft(3) == 9,467");
}

void testPosition5Perft() {
    // Position 5 (CPW: rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8)
    // Depth 1: 44
    // Depth 2: 1,486
    // Depth 3: 62,379
    auto posOpt = Position::fromFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    require(posOpt.hasValue(), "Position 5 FEN parses");
    auto pos = *posOpt;

    require(perft::perft(pos, 1) == 44ULL, "Position 5 perft(1) == 44");
    require(perft::perft(pos, 2) == 1486ULL, "Position 5 perft(2) == 1,486");
    require(perft::perft(pos, 3) == 62379ULL, "Position 5 perft(3) == 62,379");
}

void testPosition6Perft() {
    // Position 6 (CPW: r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10)
    // Depth 1: 46
    // Depth 2: 2,079
    // Depth 3: 89,890
    auto posOpt = Position::fromFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    require(posOpt.hasValue(), "Position 6 FEN parses");
    auto pos = *posOpt;

    require(perft::perft(pos, 1) == 46ULL, "Position 6 perft(1) == 46");
    require(perft::perft(pos, 2) == 2079ULL, "Position 6 perft(2) == 2,079");
    require(perft::perft(pos, 3) == 89890ULL, "Position 6 perft(3) == 89,890");
}

void testCheckmateAndStalemate() {
    // Scholar's mate checkmate position: 1. e4 e5 2. Bc4 Nc6 3. Qh5 Nf6 4. Qxf7#
    // FEN: r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4
    auto matePosOpt = Position::fromFen("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");
    require(matePosOpt.hasValue(), "Checkmate FEN parses");
    auto matePos = *matePosOpt;
    require(movegen::isInCheck(matePos), "Black is in check in Scholar's mate");
    require(movegen::evaluateGameState(matePos) == GameResult::Checkmate, "State is Checkmate");

    // Stalemate position: King on a8, White King on c7, White Queen on b6 -> Wait:
    // Standard stalemate: Black Ka8, White pawn on a7, White King on c7
    // 7k/8/8/8/8/8/8/7K w - - 0 1
    // Classic: k7/2Q5/2K5/8/8/8/8/8 b - - 0 1
    auto stalePosOpt = Position::fromFen("k7/2Q5/2K5/8/8/8/8/8 b - - 0 1");
    require(stalePosOpt.hasValue(), "Stalemate FEN parses");
    auto stalePos = *stalePosOpt;
    require(!movegen::isInCheck(stalePos), "Black is NOT in check in stalemate");
    require(movegen::evaluateGameState(stalePos) == GameResult::Stalemate, "State is Stalemate");

    // Fifty-move draw rule: halfmove clock >= 100
    auto fiftyPosOpt = Position::fromFen("8/8/8/8/8/8/8/k6K w - - 100 1");
    require(fiftyPosOpt.hasValue(), "Fifty-move FEN parses");
    auto fiftyPos = *fiftyPosOpt;
    require(movegen::evaluateGameState(fiftyPos) == GameResult::FiftyMoveDraw, "State is FiftyMoveDraw");
}

void testSpecialRulesRegressionCorpus() {
    // 1. Castling cannot happen while in check
    // White king on e1 in check by black rook on e8, castling rights KQkq
    // 4r3/8/8/8/8/8/8/R3K2R w KQ - 0 1
    auto c1Opt = Position::fromFen("4r3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    require(c1Opt.hasValue(), "Castling while in check FEN parses");
    auto c1 = *c1Opt;
    MoveList c1Moves;
    movegen::generateLegalMoves(c1, c1Moves);
    require(!c1Moves.contains(*Move::create(Square::E1, Square::G1)), "Cannot castle kingside while in check");
    require(!c1Moves.contains(*Move::create(Square::E1, Square::C1)), "Cannot castle queenside while in check");

    // 2. Castling cannot pass through check
    // Black rook attacks f1
    // 5r2/8/8/8/8/8/8/R3K2R w KQ - 0 1
    auto c2Opt = Position::fromFen("5r2/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    require(c2Opt.hasValue(), "Castling through check FEN parses");
    auto c2 = *c2Opt;
    MoveList c2Moves;
    movegen::generateLegalMoves(c2, c2Moves);
    require(!c2Moves.contains(*Move::create(Square::E1, Square::G1)), "Cannot castle kingside through check (f1 attacked)");
    require(c2Moves.contains(*Move::create(Square::E1, Square::C1)), "Can castle queenside (c1, d1 not attacked)");

    // 3. Pinned piece cannot move off pin ray
    // White king on e1, white knight on e2, black rook on e8
    // 4r3/8/8/8/8/8/4N3/4K3 w - - 0 1
    auto pinOpt = Position::fromFen("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");
    require(pinOpt.hasValue(), "Pin FEN parses");
    auto pinPos = *pinOpt;
    MoveList pinMoves;
    movegen::generateLegalMoves(pinPos, pinMoves);
    for (const auto& m : pinMoves) {
        require(m.from() != Square::E2, "Absolute pinned knight cannot move off pin line");
    }

    // 4. En-passant discovered check prevention
    // 8/8/8/K1Pp3r/8/8/8/k7 w - d6 0 1
    // If white pawn takes c5xd6 e.p., rank 5 opens and black rook on h5 attacks white king on a5!
    // So c5xd6 is illegal!
    auto epDiscovOpt = Position::fromFen("8/8/8/K1Pp3r/8/8/8/k7 w - d6 0 1");
    require(epDiscovOpt.hasValue(), "EP discovered check FEN parses");
    auto epPos = *epDiscovOpt;
    MoveList epMoves;
    movegen::generateLegalMoves(epPos, epMoves);
    require(!epMoves.contains(*Move::create(Square::C5, Square::D6)), "En-passant exposing king to discovered check is illegal");
}

void testDeterministicRandomGamesMakeUnmakeAndHashIntegrity() {
    // Run 5 deterministic random legal games across different initial positions
    // and verify every single field and Zobrist hash after every make and unmake.
    const char* testPositions[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
    };

    for (std::size_t g = 0; g < 5; ++g) {
        auto posOpt = Position::fromFen(testPositions[g]);
        require(posOpt.hasValue(), "Test position parses");
        Position pos = *posOpt;
        const std::string originalFen = pos.toFen();
        const std::uint64_t initialHash = pos.zobristHash();

        struct HistoryItem {
            Move move;
            UndoState undo;
            std::uint64_t preMoveHash;
            Color preSide;
            CastlingRights preCastling;
            Square preEp;
            std::uint32_t preHalfmove;
            std::uint32_t preFullmove;
        };

        std::vector<HistoryItem> history;

        for (int ply = 0; ply < 40; ++ply) {
            MoveList legalMoves;
            movegen::generateLegalMoves(pos, legalMoves);
            if (legalMoves.empty()) {
                break;
            }

            // Invariants before move
            require(pos.checkInvariants(), "Position invariant must hold before makeMove");
            require(pos.zobristHash() == zobrist::computeHash(pos),
                    "Incremental hash must match full recomputed hash before makeMove");

            const Move chosen = legalMoves[static_cast<std::size_t>(static_cast<std::size_t>(ply) * 13U + g * 7U) % legalMoves.size()];
            HistoryItem item;
            item.move = chosen;
            item.preMoveHash = pos.zobristHash();
            item.preSide = pos.sideToMove();
            item.preCastling = pos.castlingRights();
            item.preEp = pos.enPassantSquare();
            item.preHalfmove = pos.halfmoveClock();
            item.preFullmove = pos.fullmoveNumber();

            const bool ok = pos.makeMove(chosen, item.undo);
            require(ok, "Legal move must succeed");
            require(pos.checkInvariants(), "Position invariant must hold after makeMove");
            require(pos.zobristHash() == zobrist::computeHash(pos),
                    "Incremental hash must match full recomputed hash after makeMove");

            history.push_back(item);
        }

        // Unmake in reverse order and verify all state fields match exactly
        while (!history.empty()) {
            const HistoryItem item = history.back();
            history.pop_back();

            pos.unmakeMove(item.move, item.undo);

            require(pos.checkInvariants(), "Position invariant must hold after unmakeMove");
            require(pos.zobristHash() == item.preMoveHash, "Hash must be exactly restored after unmakeMove");
            require(pos.zobristHash() == zobrist::computeHash(pos), "Recomputed hash must match restored hash");
            require(pos.sideToMove() == item.preSide, "Side to move restored");
            require(pos.castlingRights() == item.preCastling, "Castling rights restored");
            require(pos.enPassantSquare() == item.preEp, "En passant square restored");
            require(pos.halfmoveClock() == item.preHalfmove, "Halfmove clock restored");
            require(pos.fullmoveNumber() == item.preFullmove, "Fullmove number restored");
        }

        require(pos.toFen() == originalFen, "Position FEN fully matches original after unwinding");
        require(pos.zobristHash() == initialHash, "Initial hash fully matches after unwinding");
    }
}

void testRandomLegalGamesReversibility() {
    auto pos = Position::startPosition();
    std::vector<UndoState> undoHistory;
    std::vector<Move> moveHistory;

    // Simulate 30 deterministic pseudo-random plies choosing first available move
    for (int ply = 0; ply < 30; ++ply) {
        MoveList legalMoves;
        movegen::generateLegalMoves(pos, legalMoves);
        if (legalMoves.empty()) {
            break;
        }
        // Pick move deterministically
        const Move chosen = legalMoves[static_cast<std::size_t>(ply * 7) % legalMoves.size()];
        UndoState undo;
        const bool success = pos.makeMove(chosen, undo);
        require(success, "makeMove during random simulation must succeed");
        undoHistory.push_back(undo);
        moveHistory.push_back(chosen);
    }

    // Unwind all moves and verify exact match with start position
    while (!moveHistory.empty()) {
        const Move m = moveHistory.back();
        moveHistory.pop_back();
        const UndoState u = undoHistory.back();
        undoHistory.pop_back();
        pos.unmakeMove(m, u);
    }

    require(pos.toFen() == Position::kStartPositionFen, "All moves cleanly unwound to start position");
}

}  // namespace

int main() {
    testStartPositionPerft();
    testKiwipetePerft();
    testPosition3Perft();
    testPosition4Perft();
    testPosition5Perft();
    testPosition6Perft();
    testCheckmateAndStalemate();
    testSpecialRulesRegressionCorpus();
    testRandomLegalGamesReversibility();
    testDeterministicRandomGamesMakeUnmakeAndHashIntegrity();

    std::cout << "All legal move generation and Perft certification tests passed successfully.\n";
    return EXIT_SUCCESS;
}
