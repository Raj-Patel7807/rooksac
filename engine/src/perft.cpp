#include "rooksac/perft.hpp"

#include <sstream>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/types.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {
namespace perft {

std::uint64_t perft(Position& position, int depth) noexcept {
    if (depth <= 0) {
        return 1ULL;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (depth == 1) {
        return static_cast<std::uint64_t>(legalMoves.size());
    }

    std::uint64_t nodes = 0;
    UndoState undo;
    for (const Move move : legalMoves) {
        const bool success = position.makeMove(move, undo);
        (void)success;
        nodes += perft(position, depth - 1);
        position.unmakeMove(move, undo);
    }

    return nodes;
}

PerftResult divide(Position& position, int depth) {
    PerftResult result;
    if (depth <= 0) {
        result.totalNodes = 1ULL;
        return result;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    UndoState undo;
    for (const Move move : legalMoves) {
        const bool success = position.makeMove(move, undo);
        (void)success;
        const std::uint64_t subtreeNodes = (depth > 1) ? perft(position, depth - 1) : 1ULL;
        position.unmakeMove(move, undo);

        result.divideEntries.push_back({move, subtreeNodes});
        result.totalNodes += subtreeNodes;
    }

    return result;
}

std::string formatDivide(const PerftResult& result) {
    std::ostringstream oss;
    for (const auto& entry : result.divideEntries) {
        const auto fromAlg = squareToAlgebraic(entry.move.from());
        const auto toAlg = squareToAlgebraic(entry.move.to());
        oss << fromAlg[0] << fromAlg[1] << toAlg[0] << toAlg[1];

        const auto promo = entry.move.promotion();
        if (promo != PieceType::None) {
            switch (promo) {
                case PieceType::Queen: oss << 'q'; break;
                case PieceType::Rook: oss << 'r'; break;
                case PieceType::Bishop: oss << 'b'; break;
                case PieceType::Knight: oss << 'n'; break;
                default: break;
            }
        }
        oss << ": " << entry.nodeCount << '\n';
    }
    oss << "\nTotal nodes: " << result.totalNodes << '\n';
    return oss.str();
}

}  // namespace perft
}  // namespace rooksac
