#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"
#include "rooksac/uci.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << '\n';
        std::abort();
    }
}

struct TacticalEntry {
    std::string id;
    std::string fen;
    std::string bestMoveUci;
};

std::vector<TacticalEntry> loadTacticalSuite(const std::string& path) {
    std::vector<TacticalEntry> entries;
    std::ifstream file(path);
    if (!file.is_open()) {
        file.open("../" + path);
    }
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open " << path << '\n';
        return entries;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string part;
        std::vector<std::string> parts;
        while (std::getline(ss, part, '|')) {
            // Trim whitespace
            part.erase(0, part.find_first_not_of(" \t\r\n"));
            part.erase(part.find_last_not_of(" \t\r\n") + 1);
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        if (parts.size() >= 3) {
            TacticalEntry entry;
            entry.fen = parts[0];
            // parts[1] is id=...
            if (parts[1].rfind("id=", 0) == 0) {
                entry.id = parts[1].substr(3);
            }
            // parts[2] is bm ...
            if (parts[2].rfind("bm ", 0) == 0) {
                entry.bestMoveUci = parts[2].substr(3);
            }
            entries.push_back(entry);
        }
    }
    return entries;
}

void testTacticalSuite() {
    const auto entries = loadTacticalSuite("tools/tactical_suite.epd");
    require(!entries.empty(), "Tactical test suite loaded at least one puzzle");

    std::size_t solved = 0;
    for (const auto& entry : entries) {
        auto posOpt = Position::fromFen(entry.fen);
        require(posOpt.hasValue(), "Puzzle FEN is valid");

        // Solve with depth 3 search
        search::SearchLimits limits(3);
        const auto result = search::search(*posOpt, limits);

        require(result.bestMove.has_value(), "Search found a best move");
        const std::string foundMove = uci::formatMove(*result.bestMove);
        if (foundMove == entry.bestMoveUci) {
            ++solved;
        }
    }

    std::cout << "Tactical suite results: " << solved << " / " << entries.size() << " solved at depth 3.\n";
    require(solved >= 7, "Engine solves majority of tactical suite puzzles at depth 3");
}

}  // namespace

int main() {
    testTacticalSuite();
    std::cout << "Tactical regression test suite passed.\n";
    return 0;
}
