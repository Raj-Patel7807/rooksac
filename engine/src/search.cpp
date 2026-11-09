#include "rooksac/search.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_order.hpp"
#include "rooksac/see.hpp"

namespace rooksac {
namespace search {

namespace {

// ─── Internal search context ─────────────────────────────────────────────────

struct SearchContext {
    SearchStopToken* stopToken = nullptr;
    tt::TranspositionTable* ttable = nullptr;
    std::optional<std::chrono::steady_clock::time_point> deadline;
    std::optional<std::uint64_t> maxNodes;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    bool aborted = false;

    // Killer move table, allocated per search invocation on the stack inside `search()`.
    // Pointer may be null if killer heuristic is disabled.
    KillerTable* killers = nullptr;

    // History heuristic table, allocated per search invocation on the stack inside `search()`.
    // Pointer may be null if history heuristic is disabled.
    HistoryTable* history = nullptr;

    // Active search options for this invocation.
    SearchOptions options;

    // Search-path position history for repetition detection.
    // Stack-allocated array of Zobrist keys along the current search branch.
    static constexpr std::size_t kMaxPathPlies = 256;
    std::array<std::uint64_t, kMaxPathPlies> historyPath{};
    std::size_t historyLen = 0;

    void pushKey(std::uint64_t key) noexcept {
        if (historyLen < kMaxPathPlies) {
            historyPath[historyLen++] = key;
        }
    }

    void popKey() noexcept {
        if (historyLen > 0) {
            --historyLen;
        }
    }

    [[nodiscard]] bool isRepetition(std::uint64_t key, std::uint32_t halfmoveClock) const noexcept {
        if (historyLen == 0 || halfmoveClock < 2) {
            return false;
        }
        // Positions can only repeat every 2 plies (same side to move).
        // The halfmove clock bounds how far back irreversible moves occurred.
        const std::size_t maxLookback = std::min(historyLen, static_cast<std::size_t>(halfmoveClock));
        for (std::size_t i = 2; i <= maxLookback; i += 2) {
            if (historyPath[historyLen - i] == key) {
                return true;
            }
        }
        return false;
    }

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

// ─── Quiescence search ───────────────────────────────────────────────────────

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

    const bool inCheck = movegen::isInCheck(position);

    // 1. Quiescence depth limit: enforce unconditionally before any recursive search.
    // If at the limit:
    // - If in check, verify whether legal evasions exist to detect checkmate.
    //   If evasions exist, search depth-1 evasions to evaluate legitimate post-evasion
    //   leaves, penalizing the checking pressure rather than taking an unevaded stand-pat.
    // - If not in check, return stand-pat evaluation.
    if (ply >= kMaxQsearchDepth) {
        if (inCheck) {
            MoveList evasions;
            movegen::generateLegalMoves(position, evasions);
            if (evasions.empty()) {
                return -kMateScore + static_cast<std::int32_t>(ply);
            }
            orderMoves(position, evasions);
            std::int32_t bestEvasionScore = -kInfinity;
            for (std::size_t i = 0; i < evasions.size(); ++i) {
                const Move m = evasions[i];
                UndoState undo;
                (void)position.makeMove(m, undo);
                // Directly evaluate the leaf reached after the forced evasion
                const std::int32_t evasionScore = -evaluator(position);
                position.unmakeMove(m, undo);
                if (evasionScore > bestEvasionScore) {
                    bestEvasionScore = evasionScore;
                }
                if (bestEvasionScore > alpha) {
                    alpha = bestEvasionScore;
                }
                if (alpha >= beta) {
                    break;
                }
            }
            return bestEvasionScore;
        }
        return evaluator(position);
    }

    // 2. Terminal draw: 50-move rule and insufficient material (checkmate takes precedence)
    if (position.halfmoveClock() >= 100 || eval::hasInsufficientMaterial(position)) {
        if (inCheck) {
            MoveList evasions;
            movegen::generateLegalMoves(position, evasions);
            if (evasions.empty()) {
                return -kMateScore + static_cast<std::int32_t>(ply);
            }
        }
        return kDrawScore;
    }

    // 3. Check handling in quiescence:
    // If king is in check, stand-pat is NOT permitted. All legal evasions must be searched.
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

    // 4. Not in check: Stand-pat (the side to move can choose not to capture)
    const std::int32_t standPat = evaluator(position);
    if (standPat >= beta) {
        return standPat;
    }
    if (standPat > alpha) {
        alpha = standPat;
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

        // Delta pruning: if standPat + captured_piece_value + safety_margin < alpha,
        // and it is not a promotion, this capture cannot possibly raise alpha.
        const Piece targetPiece = position.pieceAt(m.to());
        const std::int32_t victimVal = (targetPiece != Piece::None) ? eval::pieceTypeValue(typeOf(targetPiece)) : 100;
        constexpr std::int32_t kDeltaMargin = 200; // 2 pawns margin for positional swings
        if (m.promotion() == PieceType::None && (standPat + victimVal + kDeltaMargin < alpha)) {
            continue;
        }

        // SEE pruning in quiescence: skip captures with negative SEE (hanging attacks)
        if (!seeGe(position, m, 0)) {
            continue;
        }

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

// ─── Enhanced Alpha-Beta (PVS, NMP, LMR, killers, history) ──────────────────

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

    const std::uint64_t hash = position.zobristHash();

    // 1. Search-path repetition check (non-root nodes)
    if (ply > 0 && context && context->isRepetition(hash, position.halfmoveClock())) {
        return kDrawScore;
    }

    // 2. Transposition table probe
    std::optional<Move> ttMove = std::nullopt;
    if (context && context->ttable) {
        ++stats.ttProbes;
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

    // 2. Generate legal moves to detect terminal conditions
    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        return kDrawScore;
    }

    // 3. Terminal draw: 50-move rule and insufficient material check (mate takes precedence)
    if (position.halfmoveClock() >= 100 || eval::hasInsufficientMaterial(position)) {
        return kDrawScore;
    }

    // 4. Leaf node: transition to Quiescence search
    if (depth == 0) {
        return quiescenceInternal(position, alpha, beta, ply, stats, evaluator, context);
    }

    const bool inCheck  = movegen::isInCheck(position);
    const bool isPvNode = (beta - alpha > 1);  // True when window is wider than null-window

    // ── Null-move pruning ─────────────────────────────────────────────────
    // Conditions: not in check, not a PV node, sufficient depth, sufficient material.
    // The zugzwang safeguard: require at least one non-pawn piece so that passing a turn
    // is not catastrophically incorrect (e.g., king+pawns endgames are risky).
    if (context && context->options.nullMovePruning && !inCheck && !isPvNode && depth >= 2) {
        const std::uint32_t nmpR    = context->options.nullMoveReduction;
        const std::int32_t minPieces = context->options.nullMoveMinPieces;

        // Count non-pawn/king pieces for the side to move
        const Color stm         = position.sideToMove();
        const Bitboard ownKnights = position.pieces(stm, PieceType::Knight);
        const Bitboard ownBishops = position.pieces(stm, PieceType::Bishop);
        const Bitboard ownRooks   = position.pieces(stm, PieceType::Rook);
        const Bitboard ownQueens  = position.pieces(stm, PieceType::Queen);
        const std::int32_t nonPawnCount = static_cast<std::int32_t>(
            rooksac::popCount(ownKnights) + rooksac::popCount(ownBishops) +
            rooksac::popCount(ownRooks)   + rooksac::popCount(ownQueens));

        if (nonPawnCount >= minPieces && depth >= nmpR + 1) {
            UndoState undo;
            position.makeNullMove(undo);

            const std::uint32_t reducedDepth = depth - 1 - nmpR;
            const std::int32_t nullScore = -alphaBetaInternal(
                position, -beta, -(beta - 1), reducedDepth, ply + 1, stats, evaluator, context);

            position.unmakeNullMove(undo);

            if (context && context->aborted) {
                return alpha;
            }

            if (nullScore >= beta) {
                ++stats.cutoffs;
                ++stats.nmpCutoffs;
                // Avoid returning mate scores that are unreliable from null-move search
                return (nullScore >= kMateScore - kMaxMatePly) ? beta : nullScore;
            }
        }
    }

    // 5. Staged move ordering (with TT best move, killers, and history prioritised)
    KillerTable*  killers = (context && context->options.killerMoves)  ? context->killers  : nullptr;
    HistoryTable* history = (context && context->options.historyHeuristic) ? context->history : nullptr;
    orderMoves(position, legalMoves, ttMove, killers, ply, history);

    // 6. Fail-soft Alpha-Beta with PVS and LMR
    const std::int32_t originalAlpha = alpha;
    std::int32_t bestScore           = -kInfinity;
    std::optional<Move> bestMove     = std::nullopt;
    const bool usePvs = context && context->options.pvSearch;
    const bool useLmr = context && context->options.lmr;

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        if (context) {
            context->pushKey(position.zobristHash());
        }

        const bool isCapture = (m.promotion() != PieceType::None) || [&]() {
            return undo.capturedPiece != Piece::None;
        }();

        std::int32_t score = 0;

        if (i == 0 || !usePvs) {
            // First move (or PVS disabled): search with full window
            score = -alphaBetaInternal(
                position, -beta, -alpha, depth - 1, ply + 1, stats, evaluator, context);
        } else {
            // LMR: late quiet moves at sufficient depth get a depth-scaled reduction first.
            // Safety constraints:
            // - Do not reduce captures or promotions
            // - Do not reduce when in check
            // - Do not reduce moves that give check
            // - Do not reduce killer moves
            const bool givesCheck = movegen::isInCheck(position);
            const bool isKiller = (killers != nullptr && ply < SearchOptions::kMaxKillerPly) &&
                                  ((*killers)[ply][0].encoded() == m.encoded() ||
                                   (*killers)[ply][1].encoded() == m.encoded());

            bool doLmr = useLmr &&
                         !isCapture &&
                         !inCheck &&
                         !givesCheck &&
                         !isKiller &&
                         depth >= context->options.lmrMinDepth &&
                         i    >= context->options.lmrMoveThreshold;

            std::uint32_t reduction   = 0U;
            if (doLmr) {
                reduction = context->options.lmrBaseReduction + (depth / 5U);
                ++stats.lmrReductions;
            }
            std::uint32_t searchDepth = (depth > 0) ? (depth - 1) : 0;
            // Clamp reduction so we never search at negative depth
            if (reduction > searchDepth) {
                reduction = searchDepth;
            }

            // PVS: null-window search first
            score = -alphaBetaInternal(
                position, -(alpha + 1), -alpha, searchDepth - reduction, ply + 1, stats, evaluator, context);

            // Re-search conditions: either fail-high beyond alpha, or LMR was active
            if (score > alpha && (score < beta || reduction > 0)) {
                if (reduction > 0) {
                    ++stats.lmrResearches;
                }
                // Full-depth, full-window re-search
                score = -alphaBetaInternal(
                    position, -beta, -alpha, depth - 1, ply + 1, stats, evaluator, context);
            }
        }

        if (context) {
            context->popKey();
        }
        position.unmakeMove(m, undo);

        if (context && context->aborted) {
            return alpha;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove  = m;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            ++stats.cutoffs;

            // Killer move and history update: record quiet moves that caused a beta cutoff
            if (undo.capturedPiece == Piece::None && m.promotion() == PieceType::None) {
                if (killers != nullptr) {
                    storeKillerMove(*killers, ply, m);
                }
                if (history != nullptr) {
                    updateHistory(*history, position, m, depth);
                }
            }

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

// ─── Public API ───────────────────────────────────────────────────────────────

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

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            return -kMateScore + static_cast<std::int32_t>(ply);
        }
        return kDrawScore;
    }

    if (position.halfmoveClock() >= 100 || eval::hasInsufficientMaterial(position)) {
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
    result.stats.nodes    = 1;  // Root node visited
    result.stats.maxDepth = 0;
    result.stats.cutoffs  = 0;

    if (limits.depth == 0) {
        result.score    = evaluator(position);
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

    if (position.halfmoveClock() >= 100 || eval::hasInsufficientMaterial(position)) {
        result.score    = kDrawScore;
        result.bestMove = std::nullopt;
        return result;
    }

    std::int32_t bestScore       = -kInfinity;
    std::optional<Move> bestMove = legalMoves[0];

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        UndoState undo;
        (void)position.makeMove(m, undo);
        const std::int32_t score = -negamax(position, limits.depth - 1, 1, result.stats, evaluator);
        position.unmakeMove(m, undo);

        if (score > bestScore) {
            bestScore = score;
            bestMove  = m;
        }
    }

    result.bestMove       = bestMove;
    result.score          = bestScore;
    result.completedDepth = limits.depth;
    return result;
}

SearchResult search(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator,
    SearchStopToken* stopToken,
    tt::TranspositionTable* ttable,
    const SearchOptions& options,
    const DepthCallback* depthCallback) {
    SearchContext context;
    context.stopToken = stopToken;
    context.ttable    = ttable;
    context.maxNodes  = limits.maxNodes;
    context.options   = options;

    // Killer table: zero-initialized (default Move is a null/invalid move)
    KillerTable killerTable{};
    if (options.killerMoves) {
        context.killers = &killerTable;
    }

    // History table: zero-initialized on each new search invocation
    HistoryTable historyTable{};
    clearHistory(historyTable);
    if (options.historyHeuristic) {
        context.history = &historyTable;
    }

    // Time allocation setup
    std::optional<std::uint32_t> optimalTimeMs;
    if (limits.timeAllocation.has_value()) {
        optimalTimeMs   = limits.timeAllocation->optimalTimeMs;
        context.deadline = context.startTime + std::chrono::milliseconds(limits.timeAllocation->maximumTimeMs);
    } else if (limits.moveTimeMs.has_value() || limits.remainingTimeMs.has_value()) {
        TimeControl tc;
        tc.moveTimeMs      = limits.moveTimeMs;
        tc.remainingTimeMs = limits.remainingTimeMs;
        tc.incrementMs     = limits.incrementMs;
        tc.movesToGo       = limits.movesToGo;
        const TimeAllocation alloc = TimeManager::allocate(tc);
        optimalTimeMs    = alloc.optimalTimeMs;
        context.deadline = context.startTime + std::chrono::milliseconds(alloc.maximumTimeMs);
    }

    // Initialize root position in search history path
    context.pushKey(position.zobristHash());

    SearchResult lastCompletedResult;
    lastCompletedResult.stats.nodes    = 1;  // Root node visited
    lastCompletedResult.stats.maxDepth = 0;
    lastCompletedResult.stats.cutoffs  = 0;

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

    if (position.halfmoveClock() >= 100) {
        lastCompletedResult.score    = kDrawScore;
        lastCompletedResult.bestMove = std::nullopt;
        return lastCompletedResult;
    }

    if (eval::hasInsufficientMaterial(position)) {
        lastCompletedResult.score    = kDrawScore;
        lastCompletedResult.bestMove = legalMoves[0];
        lastCompletedResult.completedDepth = 1;
        return lastCompletedResult;
    }

    // Legal fallback move guaranteed before starting deep search
    lastCompletedResult.bestMove     = legalMoves[0];
    lastCompletedResult.score        = evaluator(position);
    lastCompletedResult.completedDepth = 0;

    // Single legal move optimization:
    // If there is only one legal move and we are in clock-managed play (optimalTimeMs set),
    // return immediately without wasting clock on an inevitable move.
    if (legalMoves.size() == 1U && optimalTimeMs.has_value()) {
        lastCompletedResult.completedDepth = 1;
        return lastCompletedResult;
    }

    std::optional<Move> pvMove  = std::nullopt;
    const std::uint32_t maxDepth = std::clamp(limits.depth, 1U, 64U);

    // Iterative Deepening loop: depth 1 upward
    for (std::uint32_t currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
        if (context.stopToken && context.stopToken->isStopped()) {
            lastCompletedResult.interrupted = true;
            break;
        }

        // Root move ordering: PV move from previous iteration is ordered first
        orderMoves(position, legalMoves, pvMove, context.killers, 0, context.history);

        // ── Aspiration windows ───────────────────────────────────────────────
        // Use full window for depth 1 (no prior score to center on).
        // From depth 2 onward, try a narrow window and widen progressively on failure.
        std::int32_t aspirationAlpha = -kInfinity;
        std::int32_t aspirationBeta  = kInfinity;
        std::int32_t aspirationDelta = options.aspirationDelta;

        if (options.aspirationWindows && currentDepth >= 2) {
            const std::int32_t prevScore = lastCompletedResult.score;
            aspirationAlpha = prevScore - aspirationDelta;
            aspirationBeta  = prevScore + aspirationDelta;
        }

        std::int32_t iterationBestScore       = -kInfinity;
        std::optional<Move> iterationBestMove = legalMoves[0];

        // Aspiration search loop: widen window progressively until we succeed or use full window
        bool aspirationDone = false;
        while (!aspirationDone) {
            std::int32_t alpha  = aspirationAlpha;
            const std::int32_t beta = aspirationBeta;
            iterationBestScore  = -kInfinity;
            iterationBestMove   = legalMoves[0];

            for (std::size_t i = 0; i < legalMoves.size(); ++i) {
                const Move m = legalMoves[i];
                UndoState undo;
                (void)position.makeMove(m, undo);
                context.pushKey(position.zobristHash());
                const std::int32_t score = -alphaBetaInternal(
                    position, -beta, -alpha, currentDepth - 1, 1, lastCompletedResult.stats, evaluator, &context);
                context.popKey();
                position.unmakeMove(m, undo);

                if (context.aborted) {
                    break;
                }

                if (score > iterationBestScore) {
                    iterationBestScore = score;
                    iterationBestMove  = m;
                }
                if (score > alpha) {
                    alpha = score;
                }
            }

            if (context.aborted) {
                break;
            }

            // Aspiration window result analysis with progressive widening:
            // - Fail-low (score <= aspirationAlpha): widen low side, double delta
            // - Fail-high (score >= aspirationBeta): widen high side, double delta
            // - Two consecutive failures: fall back to full window
            // - Within window: done
            if (options.aspirationWindows && currentDepth >= 2 &&
                aspirationAlpha != -kInfinity && aspirationBeta != kInfinity) {
                if (iterationBestScore <= aspirationAlpha) {
                    // Fail-low: widen downward progressively
                    aspirationDelta *= 2;
                    aspirationAlpha = aspirationAlpha - aspirationDelta;
                    if (aspirationAlpha < -kInfinity / 2) {
                        aspirationAlpha = -kInfinity;
                    }
                    // If we've already opened the window fully, stop re-searching
                    if (aspirationAlpha == -kInfinity) {
                        aspirationDone = true;
                    }
                } else if (iterationBestScore >= aspirationBeta) {
                    // Fail-high: widen upward progressively
                    aspirationDelta *= 2;
                    aspirationBeta = aspirationBeta + aspirationDelta;
                    if (aspirationBeta > kInfinity / 2) {
                        aspirationBeta = kInfinity;
                    }
                    if (aspirationBeta == kInfinity) {
                        aspirationDone = true;
                    }
                } else {
                    aspirationDone = true;
                }
            } else {
                aspirationDone = true;
            }
        }

        if (context.aborted) {
            lastCompletedResult.interrupted = true;
            break;  // Discard partial/incomplete iteration!
        }

        // Iteration fully completed: update lastCompletedResult
        lastCompletedResult.bestMove     = iterationBestMove;
        lastCompletedResult.score        = iterationBestScore;
        lastCompletedResult.completedDepth = currentDepth;
        pvMove = iterationBestMove;

        // Notify caller of completed iteration (e.g. to emit UCI info lines)
        if (depthCallback) {
            (*depthCallback)(currentDepth, iterationBestScore,
                             lastCompletedResult.stats.nodes, iterationBestMove);
        }

        // Store root position in TT
        if (ttable) {
            ttable->store(position.zobristHash(), 0, currentDepth, iterationBestScore, tt::Bound::Exact, iterationBestMove);
        }

        // Adaptive time management check:
        // In game-tree search, each subsequent depth typically takes 3x to 4x the cumulative
        // time spent so far (the effective branching factor).
        // If elapsedMs exceeds optimalTimeMs, or if elapsedMs is already more than 50% of optimalTimeMs
        // at depth >= 5 (making it mathematically improbable that the next depth will finish before optimalTimeMs),
        // we break to preserve clock. Arithmetic uses uint64_t to prevent overflow on large elapsed values.
        if (optimalTimeMs.has_value()) {
            const auto elapsedMs = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - context.startTime).count());
            if (elapsedMs >= *optimalTimeMs) {
                break;
            }
            if (currentDepth >= 5 && elapsedMs * 2ULL >= static_cast<std::uint64_t>(*optimalTimeMs)) {
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
