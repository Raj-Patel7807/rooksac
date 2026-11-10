#include "rooksac/uci.hpp"

#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
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
    if (sv.empty()) {
        return std::nullopt;
    }
    std::uint32_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec == std::errc() && ptr == sv.data() + sv.size()) {
        return val;
    }
    return std::nullopt;
}

std::optional<std::uint64_t> parseUint64(std::string_view sv) {
    if (sv.empty()) {
        return std::nullopt;
    }
    std::uint64_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec == std::errc() && ptr == sv.data() + sv.size()) {
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
    out_ << "option name Hash type spin default 64 min 1 max 1024\n";
    out_ << "option name EvalMode type combo default Classical var Classical var Neural var Hybrid\n";
    out_ << "option name ModelPath type string default <empty>\n";
    out_ << "option name UsePolicy type check default false\n";
    out_ << "option name NeuralWeight type spin default 50 min 0 max 100\n";
    out_ << "uciok\n";
    out_.flush();
}

void EngineSession::handleIsReady() {
    if (evalMode_ == neural::EvalMode::Neural || evalMode_ == neural::EvalMode::Hybrid) {
        auto& evaluator = neural::getGlobalNeuralEvaluator();
        if (evaluator.isLoaded()) {
            const auto& meta = evaluator.metadata();
            out_ << "info string model loaded: " << meta.name << " " << meta.version
                 << " (" << meta.numBlocks << " blocks, " << meta.channels << " channels, "
                 << meta.numActions << " actions)\n";
        } else if (fallbackActive_) {
            out_ << "info string model fallback active: Classical evaluation will be used\n";
        } else {
            out_ << "info string warning: EvalMode is "
                 << (evalMode_ == neural::EvalMode::Neural ? "Neural" : "Hybrid")
                 << " but no valid model is loaded\n";
        }
    }
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

    // Per-depth callback: emit UCI info lines after each completed iteration.
    const auto searchStart = std::chrono::steady_clock::now();
    const search::DepthCallback infoCallback = [&](std::uint32_t depth, std::int32_t score,
                                                    std::uint64_t nodes, std::optional<Move> bestMove) {
        const auto elapsedMs = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - searchStart).count());
        const std::uint64_t nps = (elapsedMs > 0) ? (nodes * 1000U / elapsedMs) : nodes;
        out_ << "info depth " << depth
             << " score cp " << score
             << " nodes " << nodes
             << " nps " << nps
             << " time " << elapsedMs;
        if (bestMove.has_value()) {
            out_ << " pv " << formatMove(*bestMove);
        }
        out_ << '\n';
        out_.flush();
    };

    search::SearchOptions searchOpts{};
    searchOpts.usePolicy = usePolicy_;

    const auto leafEvaluator = [this](const Position& pos) -> std::int32_t {
        if (evalMode_ == neural::EvalMode::Classical) {
            return eval::evaluateSideToMove(pos);
        }

        auto& evaluator = neural::getGlobalNeuralEvaluator();
        if (!evaluator.isLoaded()) {
            return eval::evaluateSideToMove(pos);
        }

        if (evalMode_ == neural::EvalMode::Neural) {
            return evaluator.evaluateSideToMove(pos);
        }

        // Blended / Hybrid
        const std::int32_t classical = eval::evaluateSideToMove(pos);
        const std::int32_t neural = evaluator.evaluateSideToMove(pos);
        const float w = std::clamp(neuralWeight_, 0.0f, 1.0f);
        const auto blended = static_cast<std::int32_t>(
            std::round((1.0f - w) * static_cast<float>(classical) + w * static_cast<float>(neural)));
        return blended;
    };

    const auto result = search::search(
        position_,
        limits,
        leafEvaluator,
        &stopToken_,
        &ttable_,
        searchOpts,
        &infoCallback);

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
    if (tokens.size() >= 2 && tokens[0] == "name") {
        std::string optName = tokens[1];
        std::transform(optName.begin(), optName.end(), optName.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        std::string optValue;
        if (tokens.size() >= 4 && tokens[2] == "value") {
            // Value can be multiple tokens (especially file paths with spaces)
            std::size_t valPos = args.find("value");
            if (valPos != std::string_view::npos) {
                valPos += 5;
                while (valPos < args.size() && std::isspace(static_cast<unsigned char>(args[valPos]))) {
                    ++valPos;
                }
                optValue = std::string(args.substr(valPos));
            }
        }

        if (optName == "hash") {
            if (auto mb = parseUint(optValue); mb.has_value()) {
                stopSearch();
                joinSearch();
                ttable_.resize(static_cast<std::size_t>(*mb) * 1024ULL * 1024ULL);
                return;
            }
        } else if (optName == "modelpath") {
            stopSearch();
            joinSearch();
            modelPath_ = optValue;
            auto& evaluator = neural::getGlobalNeuralEvaluator();
            const bool allowFallback = (evalMode_ != neural::EvalMode::Neural);
            const bool ok = evaluator.loadModel(modelPath_, allowFallback);
            if (!ok && !allowFallback) {
                fallbackActive_ = false;
                err_ << "info string error: failed to load model strictly from " << modelPath_ << '\n';
                out_ << "info string error: model load failed for " << modelPath_ << '\n';
                out_.flush();
            } else if (!evaluator.isLoaded() && allowFallback) {
                fallbackActive_ = true;
                out_ << "info string model load failed, classical fallback active\n";
                out_.flush();
            } else if (evaluator.isLoaded()) {
                fallbackActive_ = false;
                const auto& meta = evaluator.metadata();
                out_ << "info string loaded model " << meta.sourcePath << " ("
                     << meta.numBlocks << "b " << meta.channels << "c " << meta.numActions << "a)\n";
                out_.flush();
            }
            return;
        } else if (optName == "evalmode") {
            stopSearch();
            joinSearch();
            std::string modeLower = optValue;
            std::transform(modeLower.begin(), modeLower.end(), modeLower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (modeLower == "classical") {
                evalMode_ = neural::EvalMode::Classical;
                fallbackActive_ = false;
                return;
            } else if (modeLower == "neural") {
                evalMode_ = neural::EvalMode::Neural;
                auto& evaluator = neural::getGlobalNeuralEvaluator();
                if (!evaluator.isLoaded()) {
                    if (!modelPath_.empty()) {
                        const bool ok = evaluator.loadModel(modelPath_, /*allowFallback=*/false);
                        if (!ok) {
                            err_ << "info string error: EvalMode=Neural requires a valid model file\n";
                            out_ << "info string error: EvalMode=Neural requires a valid model file\n";
                            out_.flush();
                        }
                    } else {
                        err_ << "info string error: EvalMode=Neural requires ModelPath to be configured first\n";
                        out_ << "info string error: ModelPath not configured for neural evaluation\n";
                        out_.flush();
                    }
                }
                return;
            } else if (modeLower == "hybrid" || modeLower == "blended") {
                evalMode_ = neural::EvalMode::Blended;
                auto& evaluator = neural::getGlobalNeuralEvaluator();
                if (!evaluator.isLoaded() && !modelPath_.empty()) {
                    evaluator.loadModel(modelPath_, /*allowFallback=*/true);
                }
                fallbackActive_ = !evaluator.isLoaded();
                return;
            }
        } else if (optName == "usepolicy") {
            std::string valLower = optValue;
            std::transform(valLower.begin(), valLower.end(), valLower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            usePolicy_ = (valLower == "true" || valLower == "1");
            return;
        } else if (optName == "neuralweight") {
            if (auto weight = parseUint(optValue); weight.has_value()) {
                const auto clamped = std::clamp(*weight, 0U, 100U);
                neuralWeight_ = static_cast<float>(clamped) / 100.0f;
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
