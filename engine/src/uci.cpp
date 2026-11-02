#include "rooksac/uci.hpp"

#include <cctype>
#include <charconv>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "rooksac/benchmark.hpp"
#include "rooksac/evaluation.hpp"
#include "rooksac/legal_movegen.hpp"
#include "rooksac/material.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/time_management.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {
namespace uci {

namespace {

std::vector<std::string> splitTokens(std::string_view sv) {
    std::vector<std::string> tokens;
    std::size_t i = 0;
    while (i < sv.size()) {
        while (i < sv.size() && std::isspace(static_cast<unsigned char>(sv[i]))) {
            ++i;
        }
        if (i >= sv.size()) break;
        std::size_t start = i;
        while (i < sv.size() && !std::isspace(static_cast<unsigned char>(sv[i]))) {
            ++i;
        }
        tokens.emplace_back(sv.substr(start, i - start));
    }
    return tokens;
}

std::optional<std::uint32_t> parseUint(std::string_view sv) {
    std::uint32_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec == std::errc()) {
        return val;
    }
    return std::nullopt;
}

std::optional<std::uint64_t> parseUint64(std::string_view sv) {
    std::uint64_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec == std::errc()) {
        return val;
    }
    return std::nullopt;
}

}  // namespace

std::string formatMove(Move move) {
    const auto fromAlg = squareToAlgebraic(move.from());
    const auto toAlg = squareToAlgebraic(move.to());

    std::string res;
    res.reserve(5);
    res.push_back(fromAlg[0]);
    res.push_back(fromAlg[1]);
    res.push_back(toAlg[0]);
    res.push_back(toAlg[1]);

    const PieceType promo = move.promotion();
    switch (promo) {
        case PieceType::Queen:
            res.push_back('q');
            break;
        case PieceType::Rook:
            res.push_back('r');
            break;
        case PieceType::Bishop:
            res.push_back('b');
            break;
        case PieceType::Knight:
            res.push_back('n');
            break;
        default:
            break;
    }

    return res;
}

std::optional<Move> parseMove(Position& position, std::string_view moveStr) {
    if (moveStr.size() != 4 && moveStr.size() != 5) {
        return std::nullopt;
    }

    const auto fromSq = squareFromAlgebraic(moveStr[0], moveStr[1]);
    const auto toSq = squareFromAlgebraic(moveStr[2], moveStr[3]);
    if (!fromSq.has_value() || !toSq.has_value()) {
        return std::nullopt;
    }

    PieceType promo = PieceType::None;
    if (moveStr.size() == 5) {
        const char p = static_cast<char>(std::tolower(static_cast<unsigned char>(moveStr[4])));
        switch (p) {
            case 'q':
                promo = PieceType::Queen;
                break;
            case 'r':
                promo = PieceType::Rook;
                break;
            case 'b':
                promo = PieceType::Bishop;
                break;
            case 'n':
                promo = PieceType::Knight;
                break;
            default:
                return std::nullopt;
        }
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const Move m = legalMoves[i];
        if (m.from() == *fromSq && m.to() == *toSq && m.promotion() == promo) {
            return m;
        }
    }

    return std::nullopt;
}

EngineSession::EngineSession(std::istream& inStream, std::ostream& outStream, std::ostream& errStream)
    : in_(inStream), out_(outStream), err_(errStream) {}

EngineSession::~EngineSession() {
    stopSearch();
    joinSearch();
}

void EngineSession::run() {
    std::string line;
    while (std::getline(in_, line)) {
        if (!processCommand(line)) {
            break;
        }
    }
}

bool EngineSession::processCommand(std::string_view line) {
    const auto tokens = splitTokens(line);
    if (tokens.empty()) {
        return true;
    }

    const std::string& cmd = tokens[0];

    if (cmd == "uci") {
        handleUci();
    } else if (cmd == "isready") {
        handleIsReady();
    } else if (cmd == "ucinewgame") {
        handleUciNewGame();
    } else if (cmd == "position") {
        const std::size_t pos = line.find("position");
        const std::string_view args = line.substr(pos + 8);
        handlePosition(args);
    } else if (cmd == "go") {
        const std::size_t pos = line.find("go");
        const std::string_view args = line.substr(pos + 2);
        handleGo(args);
    } else if (cmd == "stop") {
        handleStop();
    } else if (cmd == "quit") {
        stopSearch();
        joinSearch();
        return false;
    } else if (cmd == "setoption") {
        const std::size_t pos = line.find("setoption");
        const std::string_view args = line.substr(pos + 9);
        handleSetOption(args);
    } else if (cmd == "bench") {
        // Safe bench command: runs benchmark suite and writes to errStream to keep UCI stdout strictly protocol-compliant
        err_ << "info string running benchmark suite...\n";
        const auto report = bench::runBenchmark({}, 4, 1);
        err_ << bench::formatReport(report);
        out_ << "info nodes " << report.totalNodes << " nps " << static_cast<std::uint64_t>(report.overallNps) << '\n';
        out_.flush();
    } else {
        err_ << "info string unknown command: " << cmd << '\n';
    }

    return true;
}

void EngineSession::handleUci() {
    out_ << "id name RookSac 0.1.0\n";
    out_ << "id author RookSac Authors\n";
    out_ << "option name Hash type spin default 16 min 1 max 1024\n";
    out_ << "uciok\n";
    out_.flush();
}

void EngineSession::handleIsReady() {
    out_ << "readyok\n";
    out_.flush();
}

void EngineSession::handleUciNewGame() {
    stopSearch();
    joinSearch();
    position_ = Position::startPosition();
    ttable_.clear();
}

void EngineSession::handlePosition(std::string_view args) {
    stopSearch();
    joinSearch();

    const auto tokens = splitTokens(args);
    if (tokens.empty()) {
        return;
    }

    std::size_t moveIndex = 0;
    if (tokens[0] == "startpos") {
        position_ = Position::startPosition();
        moveIndex = 1;
    } else if (tokens[0] == "fen") {
        // Collect FEN tokens until "moves" or end
        std::string fenStr;
        std::size_t i = 1;
        while (i < tokens.size() && tokens[i] != "moves") {
            if (!fenStr.empty()) fenStr.push_back(' ');
            fenStr.append(tokens[i]);
            ++i;
        }

        auto posOpt = Position::fromFen(fenStr);
        if (!posOpt.hasValue()) {
            err_ << "info string invalid FEN: " << fenStr << '\n';
            return;
        }
        position_ = *posOpt;
        moveIndex = i;
    } else {
        err_ << "info string malformed position command\n";
        return;
    }

    if (moveIndex < tokens.size() && tokens[moveIndex] == "moves") {
        ++moveIndex;
        for (std::size_t i = moveIndex; i < tokens.size(); ++i) {
            auto mOpt = parseMove(position_, tokens[i]);
            if (!mOpt.has_value()) {
                err_ << "info string illegal or malformed move: " << tokens[i] << '\n';
                return;
            }
            UndoState undo;
            static_cast<void>(position_.makeMove(*mOpt, undo));
        }
    }
}

void EngineSession::handleGo(std::string_view args) {
    stopSearch();
    joinSearch();

    const auto tokens = splitTokens(args);
    search::SearchLimits limits;
    // Default search limits for UCI if unspecified: depth 64
    limits.depth = 64;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            if (auto d = parseUint(tokens[i + 1]); d.has_value()) {
                limits.depth = *d;
            }
            ++i;
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            if (auto mt = parseUint(tokens[i + 1]); mt.has_value()) {
                limits.moveTimeMs = *mt;
            }
            ++i;
        } else if (tokens[i] == "nodes" && i + 1 < tokens.size()) {
            if (auto nd = parseUint64(tokens[i + 1]); nd.has_value()) {
                limits.maxNodes = *nd;
            }
            ++i;
        } else if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
            if (position_.sideToMove() == Color::White) {
                if (auto t = parseUint(tokens[i + 1]); t.has_value()) {
                    limits.remainingTimeMs = *t;
                }
            }
            ++i;
        } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
            if (position_.sideToMove() == Color::Black) {
                if (auto t = parseUint(tokens[i + 1]); t.has_value()) {
                    limits.remainingTimeMs = *t;
                }
            }
            ++i;
        } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
            if (position_.sideToMove() == Color::White) {
                if (auto inc = parseUint(tokens[i + 1]); inc.has_value()) {
                    limits.incrementMs = *inc;
                }
            }
            ++i;
        } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
            if (position_.sideToMove() == Color::Black) {
                if (auto inc = parseUint(tokens[i + 1]); inc.has_value()) {
                    limits.incrementMs = *inc;
                }
            }
            ++i;
        } else if (tokens[i] == "movestogo" && i + 1 < tokens.size()) {
            if (auto mtg = parseUint(tokens[i + 1]); mtg.has_value()) {
                limits.movesToGo = *mtg;
            }
            ++i;
        } else if (tokens[i] == "infinite") {
            limits.depth = 64;
            // No time or node limits
        }
    }

    stopToken_.reset();
    isSearching_.store(true, std::memory_order_relaxed);

    searchThread_ = std::make_unique<std::thread>(&EngineSession::searchWorker, this, limits);
}

void EngineSession::searchWorker(search::SearchLimits limits) {
    // If remaining time is specified without explicit timeAllocation, calculate it with TimeManager
    if (limits.remainingTimeMs.has_value() && !limits.timeAllocation.has_value()) {
        TimeControl tc;
        tc.depth = limits.depth;
        tc.maxNodes = limits.maxNodes;
        tc.moveTimeMs = limits.moveTimeMs;
        tc.remainingTimeMs = limits.remainingTimeMs;
        tc.incrementMs = limits.incrementMs;
        tc.movesToGo = limits.movesToGo;
        limits.timeAllocation = TimeManager::allocate(tc);
    }

    ttable_.advanceAge();
    const auto result = search::search(
        position_,
        limits,
        [](const Position& pos) {
            return eval::evaluateSideToMove(pos);
        },
        &stopToken_,
        &ttable_);

    // Emit bestmove strictly to stdout
    if (result.bestMove.has_value()) {
        out_ << "bestmove " << formatMove(*result.bestMove) << '\n';
    } else {
        out_ << "bestmove 0000\n";
    }
    out_.flush();

    isSearching_.store(false, std::memory_order_relaxed);
}

void EngineSession::handleStop() {
    stopSearch();
    joinSearch();
}

void EngineSession::handleSetOption(std::string_view args) {
    const auto tokens = splitTokens(args);
    // Standard format: setoption name <name> [value <val>]
    if (tokens.size() >= 4 && tokens[0] == "name" && tokens[2] == "value") {
        std::string optName = tokens[1];
        std::transform(optName.begin(), optName.end(), optName.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (optName == "hash") {
            if (auto mb = parseUint(tokens[3]); mb.has_value()) {
                stopSearch();
                joinSearch();
                ttable_.resize(static_cast<std::size_t>(*mb) * 1024ULL * 1024ULL);
                return;
            }
        }
    }

    err_ << "info string setoption received: " << args << " (unsupported option or invalid value)\n";
}

void EngineSession::stopSearch() {
    stopToken_.stop();
}

void EngineSession::joinSearch() {
    if (searchThread_ && searchThread_->joinable()) {
        searchThread_->join();
        searchThread_.reset();
    }
    isSearching_.store(false, std::memory_order_relaxed);
}

}  // namespace uci
}  // namespace rooksac
