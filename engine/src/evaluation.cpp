#include "rooksac/evaluation.hpp"

#include <algorithm>
#include <array>
#include <bit>

#include "rooksac/attacks.hpp"
#include "rooksac/bitboard.hpp"

namespace rooksac {
namespace eval {

namespace {

// Mirrors square vertically for Black's perspective: rank 1 <-> rank 8
[[nodiscard]] constexpr Square mirrorSquare(Square sq) noexcept {
    return static_cast<Square>(squareIndex(sq) ^ 56U);
}

// In standard tapered evaluation, phase weights for non-pawn pieces:
// Knight: 1, Bishop: 1, Rook: 2, Queen: 4
// Total per side: 4*1 + 2*2 + 1*4 = 12; Both sides: 24.
constexpr std::int32_t kKnightPhase = 1;
constexpr std::int32_t kBishopPhase = 1;
constexpr std::int32_t kRookPhase = 2;
constexpr std::int32_t kQueenPhase = 4;

EvalParameters initDefaultParameters() noexcept {
    EvalParameters p;

    // Standard simplified evaluation tables (PeSTO / CPW classical PSTs)
    // 64 values indexed a1..h8 (rank 1 at bottom, rank 8 at top)
    constexpr std::int32_t mgPawn[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
         98, 134,  61,  95,  68, 126,  34, -11,
         -6,   7,  26,  31,  65,  56,  25, -20,
        -14,  13,   6,  21,  23,  12,  17, -23,
        -27,  -2,  -5,  12,  17,   6,  10, -25,
        -26,  -4,  -4, -10,   3,   3,  33, -12,
        -35,  -1, -20, -23, -15,  24,  38, -22,
          0,   0,   0,   0,   0,   0,   0,   0
    };
    constexpr std::int32_t egPawn[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
        178, 173, 158, 134, 147, 132, 165, 187,
         94, 100,  85,  67,  56,  53,  82,  84,
         32,  24,  13,   5,  -2,   4,  17,  17,
         13,   9,  -3,  -7,  -7,  -8,   3,  -1,
          4,   7,  -6,   1,   0,  -5,  -1,  -8,
         13,   8,   8,  10,  13,   0,   2,  -7,
          0,   0,   0,   0,   0,   0,   0,   0
    };

    constexpr std::int32_t mgKnight[64] = {
       -167, -89, -34, -49,  61, -97, -15, -107,
        -73, -41,  72,  36,  23,  62,   7,  -17,
        -47,  60,  37,  65,  84, 129,  73,   44,
         -9,  17,  19,  53,  37,  69,  18,   22,
        -13,   4,  16,  13,  28,  19,  21,   -8,
        -23,  -9,  12,  10,  19,  17,  25,  -16,
        -29, -53, -12,  -3,  -1,  18, -14,  -19,
       -105, -21, -58, -33, -17, -28, -19,  -23
    };
    constexpr std::int32_t egKnight[64] = {
        -58, -38, -13, -28, -31, -27, -63, -99,
        -25,  -8, -25,  -2,  -9, -25, -24, -52,
        -24, -20,  10,   9,  -1,  -9, -19, -41,
        -17,   3,  22,  22,  22,  11,   8, -18,
        -18,  -6,  16,  25,  16,  17,   4, -18,
        -23,  -3,  -1,  15,  10,  -3, -20, -22,
        -42, -20, -10,  -5,  -2, -20, -23, -44,
        -29, -51, -23, -15, -22, -18, -50, -64
    };

    constexpr std::int32_t mgBishop[64] = {
        -29,   4, -82, -37, -25, -42,   7,  -8,
        -26,  16, -18, -13,  30,  59,  18, -47,
        -16,  37,  43,  40,  35,  50,  37,  -2,
         -4,   5,  19,  50,  37,  37,   7,  -2,
         -6,  13,  13,  26,  34,  12,  10,   4,
          0,  15,  15,  15,  14,  27,  18,  10,
          4,  15,  16,   0,   7,  21,  33,   1,
        -33,  -3, -14, -21, -13, -12, -39, -21
    };
    constexpr std::int32_t egBishop[64] = {
        -14, -21, -11,  -8,  -7,  -9, -17, -24,
         -8,  -4,   7, -12,  -3, -13,  -4, -16,
          2,  -8,   0,  -1,  -2,   6,   0,   4,
         -3,   9,  12,   9,  14,  10,   3,   2,
         -6,   3,  13,  19,   7,  10,  -3,  -9,
        -12,  -3,   8,  10,  13,   3,  -7, -15,
        -14, -18,  -7,  -1,   4,  -9, -15, -27,
        -23,  -9, -23,  -5,  -9, -16,  -5, -17
    };

    constexpr std::int32_t mgRook[64] = {
         32,  42,  32,  51,  63,   9,  31,  43,
         27,  32,  58,  62,  80,  67,  26,  44,
         -5,  19,  26,  36,  17,  45,  61,  16,
        -24, -11,   7,  26,  24,  35,  -8, -20,
        -36, -26, -12,  -1,   9,  -7,   6, -23,
        -45, -25, -16, -17,   3,   0,  -5, -33,
        -44, -16, -20,  -9,  -1,  11,  -6, -71,
        -19, -13,   1,  17,  16,   7, -37, -26
    };
    constexpr std::int32_t egRook[64] = {
         13,  10,  18,  15,  12,  12,   8,   5,
         11,  13,  13,  11,  -3,   3,   8,   3,
          7,   7,   7,   5,   4,  -3,  -5,  -3,
          4,   3,  13,   1,   2,   1,  -1,   2,
          3,   5,   8,   4,  -5,  -6,  -8, -11,
         -4,   0,  -5,  -1,  -7, -12,  -8, -16,
         -6,  -6,   0,   2,  -9,  -9, -11,  -3,
         -9,   2,   3,  -1,  -5, -13,   4, -20
    };

    constexpr std::int32_t mgQueen[64] = {
        -28,   0,  29,  12,  59,  44,  43,  45,
        -24, -39,  -5,   1, -16,  57,  28,  54,
        -13, -17,   7,   8,  29,  56,  47,  57,
        -27, -27, -16, -16,  -1,  17,  -2,   1,
         -9, -26,  -9, -10,  -2,  -4,   3,  -3,
        -14,   2, -11,  -2,  -5,   2,  14,   5,
        -35,  -8,  11,   2,   8,  15,  -3,   1,
         -1, -18,  -9,  10, -15, -25, -31, -50
    };
    constexpr std::int32_t egQueen[64] = {
         -9,  22,  22,  27,  27,  19,  10,  20,
        -17,  20,  32,  41,  58,  25,  30,   0,
        -20,   6,   9,  49,  47,  35,  19,   9,
          3,  22,  24,  45,  57,  40,  57,  36,
        -18,  28,  19,  47,  31,  34,  39,  18,
        -16, -27,  15,   6,   9,  17,  10,   5,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -33, -28, -22, -43,  -5, -32, -20, -41
    };

    constexpr std::int32_t mgKing[64] = {
        -65,  23,  16, -15, -56, -34,   2,  13,
         29,  -1, -20,  -7,  -8,  -4, -38, -29,
         -9,  24,   2, -16, -20,   6,  22, -22,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -49,  -1, -27, -39, -46, -44, -33, -51,
        -14, -14, -22, -46, -44, -30, -15, -27,
          1,   7,  -8, -64, -43, -16,   9,   8,
        -15,  36,  12, -54,   8, -28,  24,  14
    };
    constexpr std::int32_t egKing[64] = {
        -74, -35, -18, -18, -11,  15,   4, -17,
        -12,  17,  14,  17,  17,  38,  23,  11,
         10,  17,  23,  15,  20,  45,  44,  13,
         -8,  22,  24,  27,  26,  33,  26,   3,
        -18,  -4,  21,  24,  27,  23,   9, -11,
        -19,  -3,  11,  21,  23,  16,   7,  -9,
        -27, -11,   4,  13,  14,   4,  -5, -17,
        -53, -34, -21, -11, -28, -14, -24, -43
    };

    for (std::size_t i = 0; i < kSquareCount; ++i) {
        p.pstPawn[i] = Score{mgPawn[i], egPawn[i]};
        p.pstKnight[i] = Score{mgKnight[i], egKnight[i]};
        p.pstBishop[i] = Score{mgBishop[i], egBishop[i]};
        p.pstRook[i] = Score{mgRook[i], egRook[i]};
        p.pstQueen[i] = Score{mgQueen[i], egQueen[i]};
        p.pstKing[i] = Score{mgKing[i], egKing[i]};
    }

    return p;
}

}  // namespace

const EvalParameters& defaultParameters() noexcept {
    static const EvalParameters kDefault = initDefaultParameters();
    return kDefault;
}

std::int32_t calculateGamePhase(const Position& position) noexcept {
    std::int32_t phase = 0;

    const auto knights = std::popcount(position.pieces(Color::White, PieceType::Knight) |
                                       position.pieces(Color::Black, PieceType::Knight));
    const auto bishops = std::popcount(position.pieces(Color::White, PieceType::Bishop) |
                                       position.pieces(Color::Black, PieceType::Bishop));
    const auto rooks = std::popcount(position.pieces(Color::White, PieceType::Rook) |
                                     position.pieces(Color::Black, PieceType::Rook));
    const auto queens = std::popcount(position.pieces(Color::White, PieceType::Queen) |
                                      position.pieces(Color::Black, PieceType::Queen));

    phase += static_cast<std::int32_t>(knights) * kKnightPhase;
    phase += static_cast<std::int32_t>(bishops) * kBishopPhase;
    phase += static_cast<std::int32_t>(rooks) * kRookPhase;
    phase += static_cast<std::int32_t>(queens) * kQueenPhase;

    return std::clamp(phase, 0, kTotalPhase);
}

Score evaluateMaterialAndPST(const Position& position, const EvalParameters& params) noexcept {
    Score score{};

    // Helper lambda to accumulate piece values and PST
    auto evalPieces = [&](Color color, PieceType type, const Score& baseVal, const std::array<Score, kSquareCount>& pst) {
        Bitboard bb = position.pieces(color, type);
        while (bb != 0) {
            const Square sq = static_cast<Square>(std::countr_zero(bb));
            bb &= (bb - 1ULL);

            const Score pstScore = (color == Color::White)
                ? pst[squareIndex(sq)]
                : pst[squareIndex(mirrorSquare(sq))];

            const Score pieceTotal = baseVal + pstScore;

            if (color == Color::White) {
                score += pieceTotal;
            } else {
                score -= pieceTotal;
            }
        }
    };

    evalPieces(Color::White, PieceType::Pawn, params.pieceValues[0], params.pstPawn);
    evalPieces(Color::Black, PieceType::Pawn, params.pieceValues[0], params.pstPawn);

    evalPieces(Color::White, PieceType::Knight, params.pieceValues[1], params.pstKnight);
    evalPieces(Color::Black, PieceType::Knight, params.pieceValues[1], params.pstKnight);

    evalPieces(Color::White, PieceType::Bishop, params.pieceValues[2], params.pstBishop);
    evalPieces(Color::Black, PieceType::Bishop, params.pieceValues[2], params.pstBishop);

    evalPieces(Color::White, PieceType::Rook, params.pieceValues[3], params.pstRook);
    evalPieces(Color::Black, PieceType::Rook, params.pieceValues[3], params.pstRook);

    evalPieces(Color::White, PieceType::Queen, params.pieceValues[4], params.pstQueen);
    evalPieces(Color::Black, PieceType::Queen, params.pieceValues[4], params.pstQueen);

    // King PST (Base value is zero)
    evalPieces(Color::White, PieceType::King, Score{0, 0}, params.pstKing);
    evalPieces(Color::Black, PieceType::King, Score{0, 0}, params.pstKing);

    return score;
}

Score evaluatePawns(const Position& position, const EvalParameters& params) noexcept {
    Score score{};

    const Bitboard whitePawns = position.pieces(Color::White, PieceType::Pawn);
    const Bitboard blackPawns = position.pieces(Color::Black, PieceType::Pawn);

    constexpr Bitboard fileMasks[8] = {
        kFileA, kFileB, kFileC, kFileD, kFileE, kFileF, kFileG, kFileH
    };

    constexpr Bitboard adjacentFileMasks[8] = {
        kFileB,
        kFileA | kFileC,
        kFileB | kFileD,
        kFileC | kFileE,
        kFileD | kFileF,
        kFileE | kFileG,
        kFileF | kFileH,
        kFileG
    };

    // 1. Doubled and Isolated Pawns per file
    for (int f = 0; f < 8; ++f) {
        const int wCount = std::popcount(whitePawns & fileMasks[f]);
        if (wCount > 1) {
            score += params.doubledPawn * (wCount - 1);
        }
        if (wCount > 0 && (whitePawns & adjacentFileMasks[f]) == 0) {
            score += params.isolatedPawn * wCount;
        }

        const int bCount = std::popcount(blackPawns & fileMasks[f]);
        if (bCount > 1) {
            score -= params.doubledPawn * (bCount - 1);
        }
        if (bCount > 0 && (blackPawns & adjacentFileMasks[f]) == 0) {
            score -= params.isolatedPawn * bCount;
        }
    }

    // 2. Passed pawns
    // White passed pawn: no black pawns on same file or adjacent files in higher ranks
    Bitboard wPawns = whitePawns;
    while (wPawns != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wPawns));
        wPawns &= (wPawns - 1ULL);

        const int f = static_cast<int>(fileOf(sq));
        const int r = static_cast<int>(rankOf(sq));

        Bitboard frontSpan = 0;
        for (int frontRank = r + 1; frontRank < 8; ++frontRank) {
            frontSpan |= (kRank1 << (static_cast<std::uint32_t>(frontRank) * 8U));
        }

        const Bitboard blockingSpan = frontSpan & (fileMasks[f] | adjacentFileMasks[f]);
        if ((blackPawns & blockingSpan) == 0) {
            score += params.passedPawnByRank[r];
        }
    }

    // Black passed pawn: no white pawns on same file or adjacent files in lower ranks
    Bitboard bPawns = blackPawns;
    while (bPawns != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bPawns));
        bPawns &= (bPawns - 1ULL);

        const int f = static_cast<int>(fileOf(sq));
        const int r = static_cast<int>(rankOf(sq));

        Bitboard frontSpan = 0;
        for (int frontRank = r - 1; frontRank >= 0; --frontRank) {
            frontSpan |= (kRank1 << (static_cast<std::uint32_t>(frontRank) * 8U));
        }

        const Bitboard blockingSpan = frontSpan & (fileMasks[f] | adjacentFileMasks[f]);
        if ((whitePawns & blockingSpan) == 0) {
            score -= params.passedPawnByRank[7 - r];
        }
    }

    return score;
}

Score evaluateKingSafety(const Position& position, const EvalParameters& params) noexcept {
    Score score{};

    const Bitboard whitePawns = position.pieces(Color::White, PieceType::Pawn);
    const Bitboard blackPawns = position.pieces(Color::Black, PieceType::Pawn);

    // Evaluate shield around White King
    const Bitboard wKingBb = position.pieces(Color::White, PieceType::King);
    if (wKingBb != 0) {
        const Square wKingSq = static_cast<Square>(std::countr_zero(wKingBb));
        const int f = static_cast<int>(fileOf(wKingSq));
        const int r = static_cast<int>(rankOf(wKingSq));

        // Shield is only critical in the opening/middlegame when king is castled or on ranks 1-2
        if (r <= 2) {
            const int minF = std::max(0, f - 1);
            const int maxF = std::min(7, f + 1);

            for (int shieldFile = minF; shieldFile <= maxF; ++shieldFile) {
                const Bitboard fileMask = kFileA << shieldFile;
                // Check if friendly shield pawn exists on rank 2 or 3
                const Bitboard shieldZone = fileMask & (kRank2 | kRank3);
                if ((whitePawns & shieldZone) == 0) {
                    score += params.kingMissingShieldPawn;
                }
                // Check if file is semi-open (no friendly pawns at all on this file)
                if ((whitePawns & fileMask) == 0) {
                    score += params.kingSemiOpenFile;
                }
            }
        }
    }

    // Evaluate shield around Black King
    const Bitboard bKingBb = position.pieces(Color::Black, PieceType::King);
    if (bKingBb != 0) {
        const Square bKingSq = static_cast<Square>(std::countr_zero(bKingBb));
        const int f = static_cast<int>(fileOf(bKingSq));
        const int r = static_cast<int>(rankOf(bKingSq));

        if (r >= 5) {
            const int minF = std::max(0, f - 1);
            const int maxF = std::min(7, f + 1);

            for (int shieldFile = minF; shieldFile <= maxF; ++shieldFile) {
                const Bitboard fileMask = kFileA << shieldFile;
                const Bitboard shieldZone = fileMask & (kRank7 | kRank6);
                if ((blackPawns & shieldZone) == 0) {
                    score -= params.kingMissingShieldPawn;
                }
                if ((blackPawns & fileMask) == 0) {
                    score -= params.kingSemiOpenFile;
                }
            }
        }
    }

    return score;
}

Score evaluatePiecesAndMobility(const Position& position, const EvalParameters& params) noexcept {
    Score score{};

    const Bitboard totalOcc = position.totalOccupancy();
    const Bitboard whiteOcc = position.colorOccupancy(Color::White);
    const Bitboard blackOcc = position.colorOccupancy(Color::Black);

    const Bitboard whitePawns = position.pieces(Color::White, PieceType::Pawn);
    const Bitboard blackPawns = position.pieces(Color::Black, PieceType::Pawn);

    // Squares attacked by enemy pawns (mobility avoids these to model safe mobility)
    Bitboard blackPawnAttacks = 0;
    Bitboard bp = blackPawns;
    while (bp != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bp));
        bp &= (bp - 1ULL);
        blackPawnAttacks |= attacks::pawnAttacks(sq, Color::Black);
    }

    Bitboard whitePawnAttacks = 0;
    Bitboard wp = whitePawns;
    while (wp != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wp));
        wp &= (wp - 1ULL);
        whitePawnAttacks |= attacks::pawnAttacks(sq, Color::White);
    }

    // 1. Bishop Pair
    if (std::popcount(position.pieces(Color::White, PieceType::Bishop)) >= 2) {
        score += params.bishopPair;
    }
    if (std::popcount(position.pieces(Color::Black, PieceType::Bishop)) >= 2) {
        score -= params.bishopPair;
    }

    // 2. Rook open and semi-open files
    constexpr Bitboard fileMasks[8] = {
        kFileA, kFileB, kFileC, kFileD, kFileE, kFileF, kFileG, kFileH
    };

    auto evalRookActivity = [&](Color color) {
        Bitboard rooks = position.pieces(color, PieceType::Rook);
        while (rooks != 0) {
            const Square sq = static_cast<Square>(std::countr_zero(rooks));
            rooks &= (rooks - 1ULL);

            const int f = static_cast<int>(fileOf(sq));
            const bool ownPawn = (color == Color::White ? (whitePawns & fileMasks[f]) : (blackPawns & fileMasks[f])) != 0;
            const bool enemyPawn = (color == Color::White ? (blackPawns & fileMasks[f]) : (whitePawns & fileMasks[f])) != 0;

            if (!ownPawn && !enemyPawn) {
                if (color == Color::White) score += params.rookOnOpenFile;
                else score -= params.rookOnOpenFile;
            } else if (!ownPawn && enemyPawn) {
                if (color == Color::White) score += params.rookOnSemiOpenFile;
                else score -= params.rookOnSemiOpenFile;
            }
        }
    };
    evalRookActivity(Color::White);
    evalRookActivity(Color::Black);

    // 3. Safe Piece Mobility
    // White Knights
    Bitboard wKnights = position.pieces(Color::White, PieceType::Knight);
    while (wKnights != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wKnights));
        wKnights &= (wKnights - 1ULL);
        const Bitboard att = attacks::knightAttacks(sq) & ~whiteOcc & ~blackPawnAttacks;
        score += params.knightMobility * std::popcount(att);
    }

    // Black Knights
    Bitboard bKnights = position.pieces(Color::Black, PieceType::Knight);
    while (bKnights != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bKnights));
        bKnights &= (bKnights - 1ULL);
        const Bitboard att = attacks::knightAttacks(sq) & ~blackOcc & ~whitePawnAttacks;
        score -= params.knightMobility * std::popcount(att);
    }

    // White Bishops
    Bitboard wBishops = position.pieces(Color::White, PieceType::Bishop);
    while (wBishops != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wBishops));
        wBishops &= (wBishops - 1ULL);
        const Bitboard att = attacks::bishopAttacks(sq, totalOcc) & ~whiteOcc & ~blackPawnAttacks;
        score += params.bishopMobility * std::popcount(att);
    }

    // Black Bishops
    Bitboard bBishops = position.pieces(Color::Black, PieceType::Bishop);
    while (bBishops != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bBishops));
        bBishops &= (bBishops - 1ULL);
        const Bitboard att = attacks::bishopAttacks(sq, totalOcc) & ~blackOcc & ~whitePawnAttacks;
        score -= params.bishopMobility * std::popcount(att);
    }

    // White Rooks
    Bitboard wRooks = position.pieces(Color::White, PieceType::Rook);
    while (wRooks != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wRooks));
        wRooks &= (wRooks - 1ULL);
        const Bitboard att = attacks::rookAttacks(sq, totalOcc) & ~whiteOcc & ~blackPawnAttacks;
        score += params.rookMobility * std::popcount(att);
    }

    // Black Rooks
    Bitboard bRooks = position.pieces(Color::Black, PieceType::Rook);
    while (bRooks != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bRooks));
        bRooks &= (bRooks - 1ULL);
        const Bitboard att = attacks::rookAttacks(sq, totalOcc) & ~blackOcc & ~whitePawnAttacks;
        score -= params.rookMobility * std::popcount(att);
    }

    // White Queens
    Bitboard wQueens = position.pieces(Color::White, PieceType::Queen);
    while (wQueens != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(wQueens));
        wQueens &= (wQueens - 1ULL);
        const Bitboard att = attacks::queenAttacks(sq, totalOcc) & ~whiteOcc & ~blackPawnAttacks;
        score += params.queenMobility * std::popcount(att);
    }

    // Black Queens
    Bitboard bQueens = position.pieces(Color::Black, PieceType::Queen);
    while (bQueens != 0) {
        const Square sq = static_cast<Square>(std::countr_zero(bQueens));
        bQueens &= (bQueens - 1ULL);
        const Bitboard att = attacks::queenAttacks(sq, totalOcc) & ~blackOcc & ~whitePawnAttacks;
        score -= params.queenMobility * std::popcount(att);
    }

    return score;
}

EvalBreakdown evaluateBreakdown(const Position& position, const EvalParameters& params) noexcept {
    EvalBreakdown b;
    b.materialAndPst = evaluateMaterialAndPST(position, params);
    b.pawns = evaluatePawns(position, params);
    b.kingSafety = evaluateKingSafety(position, params);
    b.piecesAndMobility = evaluatePiecesAndMobility(position, params);
    b.phase = calculateGamePhase(position);

    const Score totalScore = b.materialAndPst + b.pawns + b.kingSafety + b.piecesAndMobility;
    b.blendedTotal = blendScore(totalScore, b.phase);
    b.sideToMoveScore = (position.sideToMove() == Color::White) ? b.blendedTotal : -b.blendedTotal;
    return b;
}

std::int32_t evaluate(const Position& position, const EvalParameters& params) noexcept {
    Score totalScore{};

    // 1. Material and Piece-Square Tables
    totalScore += evaluateMaterialAndPST(position, params);

    // 2. Pawn structure
    totalScore += evaluatePawns(position, params);

    // 3. King safety
    totalScore += evaluateKingSafety(position, params);

    // 4. Piece activity and mobility
    totalScore += evaluatePiecesAndMobility(position, params);

    // 5. Game phase and tapered interpolation
    const std::int32_t phase = calculateGamePhase(position);
    return blendScore(totalScore, phase);
}

std::int32_t evaluateSideToMove(const Position& position, const EvalParameters& params) noexcept {
    const std::int32_t score = evaluate(position, params);
    return (position.sideToMove() == Color::White) ? score : -score;
}

}  // namespace eval
}  // namespace rooksac
