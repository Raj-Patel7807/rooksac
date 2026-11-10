#pragma once

#include <atomic>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "rooksac/move.hpp"
#include "rooksac/neural_evaluator.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"
#include "rooksac/transposition_table.hpp"

namespace rooksac {
namespace uci {

/// \brief Formats a Move into standard UCI algebraic coordinate notation (e.g., "e2e4", "e7e8q").
[[nodiscard]] std::string formatMove(Move move);

/// \brief Parses a coordinate notation string (e.g. "e2e4", "e7e8q") into a Move.
/// Validates against legal moves in the given position to ensure legality.
/// Returns std::nullopt if the move is malformed or illegal in the position.
[[nodiscard]] std::optional<Move> parseMove(Position& position, std::string_view moveStr);

/// \brief State and command processor for the Universal Chess Interface (UCI).
/// Manages standard input line parsing, position updates, search lifecycles, and formatting protocol output.
class EngineSession {
public:
    /// \brief Constructs a session communicating via provided input and output streams.
    /// Diagnostics/logging are sent to errorStream (stderr), preserving outStream (stdout) purely for UCI.
    explicit EngineSession(std::istream& inStream, std::ostream& outStream, std::ostream& errStream);

    ~EngineSession();

    EngineSession(const EngineSession&) = delete;
    EngineSession& operator=(const EngineSession&) = delete;
    EngineSession(EngineSession&&) = delete;
    EngineSession& operator=(EngineSession&&) = delete;

    /// \brief Main event loop processing UCI commands until EOF or "quit".
    void run();

    /// \brief Processes a single UCI command line. Returns false if "quit" was received.
    bool processCommand(std::string_view line);

    /// \brief Aborts active search asynchronously.
    void stopSearch();

    /// \brief Waits for any active search thread to finish cleanly.
    void joinSearch();

    /// \brief Returns a reference to the active position (primarily for testing).
    [[nodiscard]] const Position& currentPosition() const noexcept { return position_; }

private:
    void handleUci();
    void handleIsReady();
    void handleUciNewGame();
    void handlePosition(std::string_view args);
    void handleGo(std::string_view args);
    void handleStop();
    void handleSetOption(std::string_view args);

    void searchWorker(search::SearchLimits limits);

    std::istream& in_;
    std::ostream& out_;
    std::ostream& err_;

    Position position_{Position::startPosition()};
    tt::TranspositionTable ttable_{tt::TranspositionTable::kDefaultSizeBytes};

    search::SearchStopToken stopToken_{};
    std::unique_ptr<std::thread> searchThread_{nullptr};
    std::atomic<bool> isSearching_{false};

    // Configuration options
    neural::EvalMode evalMode_ = neural::EvalMode::Classical;
    std::string modelPath_{};
    bool usePolicy_ = false;
    float neuralWeight_ = 0.5f;
    bool fallbackActive_ = false;
};

}  // namespace uci
}  // namespace rooksac
