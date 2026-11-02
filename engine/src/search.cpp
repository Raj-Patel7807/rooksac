#include "rooksac/search.hpp"

#include <cstdint>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_order.hpp"

namespace rooksac {
namespace search {

std::int32_t negamax(
    Position& position,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator) {
    ++stats.nodes;
    if (ply > stats.maxDepth) {
        stats.maxDepth = ply;
    }

    // 1. Terminal draw: 50-move rule check (evaluated before branching)
    if (position.halfmoveClock() >= 100) {
        return kDrawScore;
    }

    // 2. Generate legal moves to detect terminal conditions (checkmate/stalemate)
    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            // Checkmated: penalize with distance from root to prefer shorter mates and delay loss
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        // Stalemate: game ends in draw
        return kDrawScore;
    }

    // 3. Leaf node: static evaluation is evaluated only after confirming state is not terminal
    if (depth == 0) {
        return evaluator(position);
    }

    // 4. Recursive exploration
    std::int32_t bestScore = -kInfinity;

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -negamax(position, depth - 1, ply + 1, stats, evaluator);
        position.unmakeMove(m, undo);

        if (score > bestScore) {
            bestScore = score;
        }
    }

    return bestScore;
}

std::int32_t alphaBeta(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator) {
    ++stats.nodes;
    if (ply > stats.maxDepth) {
        stats.maxDepth = ply;
    }

    // 1. Terminal draw: 50-move rule check
    if (position.halfmoveClock() >= 100) {
        return kDrawScore;
    }

    // 2. Generate legal moves to detect terminal conditions
    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        return kDrawScore;
    }

    // 3. Leaf node
    if (depth == 0) {
        return evaluator(position);
    }

    // 4. Staged move ordering: captures and promotions first
    orderMoves(position, legalMoves);

    // 5. Fail-soft Alpha-Beta recursion
    std::int32_t bestScore = -kInfinity;

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -alphaBeta(
            position, -beta, -alpha, depth - 1, ply + 1, stats, evaluator);
        position.unmakeMove(m, undo);

        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            ++stats.cutoffs;
            break;  // Beta cutoff (fail-high)
        }
    }

    return bestScore;
}

SearchResult searchMinimax(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator) {
    SearchResult result;
    result.stats.nodes = 1;  // Root node visited
    result.stats.maxDepth = 0;
    result.stats.cutoffs = 0;

    if (limits.depth == 0) {
        result.score = evaluator(position);
        result.bestMove = std::nullopt;
        return result;
    }

    if (position.halfmoveClock() >= 100) {
        result.score = kDrawScore;
        result.bestMove = std::nullopt;
        return result;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            result.score = -kMateScore;
        } else {
            result.score = kDrawScore;
        }
        result.bestMove = std::nullopt;
        return result;
    }

    std::int32_t bestScore = -kInfinity;
    std::optional<Move> bestMove = legalMoves[0];

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -negamax(position, limits.depth - 1, 1, result.stats, evaluator);
        position.unmakeMove(m, undo);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
    }

    result.bestMove = bestMove;
    result.score = bestScore;
    return result;
}

SearchResult search(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator) {
    SearchResult result;
    result.stats.nodes = 1;  // Root node visited
    result.stats.maxDepth = 0;
    result.stats.cutoffs = 0;

    if (limits.depth == 0) {
        result.score = evaluator(position);
        result.bestMove = std::nullopt;
        return result;
    }

    if (position.halfmoveClock() >= 100) {
        result.score = kDrawScore;
        result.bestMove = std::nullopt;
        return result;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            result.score = -kMateScore;
        } else {
            result.score = kDrawScore;
        }
        result.bestMove = std::nullopt;
        return result;
    }

    // Staged move ordering at the root node
    orderMoves(position, legalMoves);

    std::int32_t alpha = -kInfinity;
    const std::int32_t beta = kInfinity;
    std::int32_t bestScore = -kInfinity;
    std::optional<Move> bestMove = legalMoves[0];

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -alphaBeta(
            position, -beta, -alpha, limits.depth - 1, 1, result.stats, evaluator);
        position.unmakeMove(m, undo);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    result.bestMove = bestMove;
    result.score = bestScore;
    return result;
}

}  // namespace search
}  // namespace rooksac
