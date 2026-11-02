#include <iostream>

#include "rooksac/uci.hpp"

int main() {
    // Fast I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    rooksac::uci::EngineSession session(std::cin, std::cout, std::cerr);
    session.run();

    return 0;
}
