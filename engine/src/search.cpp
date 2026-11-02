#include "rooksac/search.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_order.hpp"

namespace rooksac {
namespace search {

namespace {

struct SearchContext {
    SearchStopToken* stopToken = nullptr;
    tt::TranspositionTable* ttable = nullptr;
    std::optional<std::chrono::steady_clock::time_point> deadline;
    std::optional<std::uint64_t> maxNodes;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    bool aborted = false;

    [[nodiscard]] bool checkStop(std::uint64_t currentNodes) noexcept {
        if (aborted) {
            return true;
        }
        if ((currentNodes & (kStopCheckInterval - 1U)) == 0) {
            if (stopToken && stopToken->isStopped()) {
                aborted = true;
                return true;
            }
            if (maxNodes.has_value() && currentNodes >= *maxNodes) {
                aborted = true;
                return true;
            }
            if (deadline.has_value() && std::chrono::steady_clock::now() >= *deadline) {
                aborted = true;
                return true;
            }
        }
        return false;
    }
};

std::int32_t quiescenceInternal(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator,
    SearchContext* context) {
    ++stats.nodes;
    ++stats.qnodes;
    if (ply > stats.maxDepth) {
        stats.maxDepth = ply;
    }

    if (context && context->checkStop(stats.nodes)) {
        return alpha;
    }

    // 1. Terminal draw: 50-move rule check
    if (position.halfmoveClock() >= 100) {
        return kDrawScore;
    }

    // 2. Check handling in quiescence:
    // If king is in check, stand-pat is NOT permitted. All legal evasions must be searched.
    const bool inCheck = movegen::isInCheck(position);
    if (inCheck) {
        MoveList evasions;
        movegen::generateLegalMoves(position, evasions);

        if (evasions.empty()) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }

        orderMoves(position, evasions);

        std::int32_t bestScore = -kInfinity;
        for (std::size_t i = 0; i < evasions.size(); ++i) {
            const Move m = evasions[i];
            UndoState undo;
            (void)position.makeMove(m, undo);
            const std::int32_t score = -quiescenceInternal(
                position, -beta, -alpha, ply + 1, stats, evaluator, context);
            position.unmakeMove(m, undo);

            if (context && context->aborted) {
                return alpha;
            }

            if (score > bestScore) {
                bestScore = score;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (alpha >= beta) {
                ++stats.cutoffs;
                break;
            }
        }
        return bestScore;
    }

    // 3. Not in check: Stand-pat (the side to move can choose not to capture)
    const std::int32_t standPat = evaluator(position);
    if (standPat >= beta) {
        return standPat;
    }
    if (standPat > alpha) {
        alpha = standPat;
    }

    // 4. Maximum quiescence search depth limit safeguard
    if (ply >= kMaxQsearchDepth) {
        return standPat;
    }

    // 5. Generate and search only captures and promotions
    MoveList captures;
    movegen::generateLegalCaptures(position, captures);
    if (captures.empty()) {
        return standPat;
    }

    // Order captures by MVV-LVA priority
    orderMoves(position, captures);

    std::int32_t bestScore = standPat;
    for (std::size_t i = 0; i < captures.size(); ++i) {
        const Move m = captures[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -quiescenceInternal(
            position, -beta, -alpha, ply + 1, stats, evaluator, context);
        position.unmakeMove(m, undo);

        if (context && context->aborted) {
            return alpha;
        }

        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            ++stats.cutoffs;
            break;
        }
    }

    return bestScore;
}

std::int32_t alphaBetaInternal(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator,
    SearchContext* context) {
    ++stats.nodes;
    if (ply > stats.maxDepth) {
        stats.maxDepth = ply;
    }

    if (context && context->checkStop(stats.nodes)) {
        return alpha;
    }

    // 1. Terminal draw: 50-move rule check
    if (position.halfmoveClock() >= 100) {
        return kDrawScore;
    }

    // 2. Transposition table probe
    const std::uint64_t hash = position.zobristHash();
    std::optional<Move> ttMove = std::nullopt;
    if (context && context->ttable) {
        const auto ttProbe = context->ttable->probe(hash, ply);
        if (ttProbe.hit) {
            ++stats.ttHits;
            ttMove = ttProbe.move;
            // Can we use TT score for cutoff?
            // Only if depth of searched entry is >= current search depth
            if (ttProbe.depth >= depth) {
                if (ttProbe.bound == tt::Bound::Exact) {
                    ++stats.ttCutoffs;
                    return ttProbe.score;
                }
                if (ttProbe.bound == tt::Bound::Lower && ttProbe.score >= beta) {
                    ++stats.ttCutoffs;
                    return ttProbe.score;
                }
                if (ttProbe.bound == tt::Bound::Upper && ttProbe.score <= alpha) {
                    ++stats.ttCutoffs;
                    return ttProbe.score;
                }
            }
        }
    }

    // 3. Generate legal moves to detect terminal conditions
    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        return kDrawScore;
    }

    // 4. Leaf node: transition to Quiescence search
    if (depth == 0) {
        return quiescenceInternal(position, alpha, beta, ply, stats, evaluator, context);
    }

    // 5. Staged move ordering (with TT best move prioritized)
    orderMoves(position, legalMoves, ttMove);

    // 6. Fail-soft Alpha-Beta recursion
    const std::int32_t originalAlpha = alpha;
    std::int32_t bestScore = -kInfinity;
    std::optional<Move> bestMove = std::nullopt;

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -alphaBetaInternal(
            position, -beta, -alpha, depth - 1, ply + 1, stats, evaluator, context);
        position.unmakeMove(m, undo);

        if (context && context->aborted) {
            return alpha;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            ++stats.cutoffs;
            break;  // Beta cutoff (fail-high)
        }
    }

    // 7. Store result in Transposition Table if not aborted
    if (context && context->ttable && (!context->aborted)) {
        tt::Bound bound = tt::Bound::Exact;
        if (bestScore <= originalAlpha) {
            bound = tt::Bound::Upper;
        } else if (bestScore >= beta) {
            bound = tt::Bound::Lower;
        }
        context->ttable->store(hash, ply, depth, bestScore, bound, bestMove);
    }

    return bestScore;
}

}  // namespace

std::int32_t quiescence(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator) {
    return quiescenceInternal(position, alpha, beta, ply, stats, evaluator, nullptr);
}

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

    if (position.halfmoveClock() >= 100) {
        return kDrawScore;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        return kDrawScore;
    }

    if (depth == 0) {
        return evaluator(position);
    }

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
    return alphaBetaInternal(position, alpha, beta, depth, ply, stats, evaluator, nullptr);
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
    result.completedDepth = limits.depth;
    return result;
}

SearchResult search(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator,
    SearchStopToken* stopToken,
    tt::TranspositionTable* ttable) {
    SearchContext context;
    context.stopToken = stopToken;
    context.ttable = ttable;
    context.maxNodes = limits.maxNodes;

    // Time allocation setup
    std::optional<std::uint32_t> optimalTimeMs;
    if (limits.timeAllocation.has_value()) {
        optimalTimeMs = limits.timeAllocation->optimalTimeMs;
        context.deadline = context.startTime + std::chrono::milliseconds(limits.timeAllocation->maximumTimeMs);
    } else if (limits.moveTimeMs.has_value() || limits.remainingTimeMs.has_value()) {
        TimeControl tc;
        tc.moveTimeMs = limits.moveTimeMs;
        tc.remainingTimeMs = limits.remainingTimeMs;
        tc.incrementMs = limits.incrementMs;
        tc.movesToGo = limits.movesToGo;
        const TimeAllocation alloc = TimeManager::allocate(tc);
        optimalTimeMs = alloc.optimalTimeMs;
        context.deadline = context.startTime + std::chrono::milliseconds(alloc.maximumTimeMs);
    }

    SearchResult lastCompletedResult;
    lastCompletedResult.stats.nodes = 1;  // Root node visited
    lastCompletedResult.stats.maxDepth = 0;
    lastCompletedResult.stats.cutoffs = 0;

    if (position.halfmoveClock() >= 100) {
        lastCompletedResult.score = kDrawScore;
        lastCompletedResult.bestMove = std::nullopt;
        return lastCompletedResult;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            lastCompletedResult.score = -kMateScore;
        } else {
            lastCompletedResult.score = kDrawScore;
        }
        lastCompletedResult.bestMove = std::nullopt;
        return lastCompletedResult;
    }

    // Legal fallback move guaranteed before starting deep search
    lastCompletedResult.bestMove = legalMoves[0];
    lastCompletedResult.score = evaluator(position);
    lastCompletedResult.completedDepth = 0;

    std::optional<Move> pvMove = std::nullopt;
    const std::uint32_t maxDepth = std::clamp(limits.depth, 1U, 64U);

    // Iterative Deepening loop: depth 1 upward
    for (std::uint32_t currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
        if (context.stopToken && context.stopToken->isStopped()) {
            lastCompletedResult.interrupted = true;
            break;
        }

        // Root move ordering: PV move from previous iteration is ordered first
        orderMoves(position, legalMoves, pvMove);

        std::int32_t alpha = -kInfinity;
        const std::int32_t beta = kInfinity;
        std::int32_t iterationBestScore = -kInfinity;
        std::optional<Move> iterationBestMove = legalMoves[0];

        for (std::size_t i = 0; i < legalMoves.size(); ++i) {
            const Move m = legalMoves[i];
            UndoState undo;
            (void)position.makeMove(m, undo);
            const std::int32_t score = -alphaBetaInternal(
                position, -beta, -alpha, currentDepth - 1, 1, lastCompletedResult.stats, evaluator, &context);
            position.unmakeMove(m, undo);

            if (context.aborted) {
                break;
            }

            if (score > iterationBestScore) {
                iterationBestScore = score;
                iterationBestMove = m;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        if (context.aborted) {
            lastCompletedResult.interrupted = true;
            break;  // Discard partial/incomplete iteration!
        }

        // Iteration fully completed: update lastCompletedResult
        lastCompletedResult.bestMove = iterationBestMove;
        lastCompletedResult.score = iterationBestScore;
        lastCompletedResult.completedDepth = currentDepth;
        pvMove = iterationBestMove;

        // Store root position in TT
        if (ttable) {
            ttable->store(position.zobristHash(), 0, currentDepth, iterationBestScore, tt::Bound::Exact, iterationBestMove);
        }

        // If optimal thinking time reached, do not start next iteration
        if (optimalTimeMs.has_value()) {
            const auto elapsedMs = static_cast<std::uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - context.startTime).count());
            if (elapsedMs >= *optimalTimeMs) {
                break;
            }
        }

        // Stop if a checkmate was found
        if (isMateScore(iterationBestScore)) {
            break;
        }
    }

    return lastCompletedResult;
}

}  // namespace search
}  // namespace rooksac
