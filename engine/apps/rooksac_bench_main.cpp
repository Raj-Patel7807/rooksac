#include <iostream>
#include <string>

#include "rooksac/benchmark.hpp"

int main(int argc, char* argv[]) {
    std::uint32_t depth = 4;
    std::uint32_t repetitions = 1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--depth" && i + 1 < argc) {
            depth = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--reps" && i + 1 < argc) {
            repetitions = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        }
    }

    std::cout << "Running RookSac benchmark (depth=" << depth
              << ", reps=" << repetitions << ")...\n";

    const auto report = rooksac::bench::runBenchmark({}, depth, repetitions);
    std::cout << rooksac::bench::formatReport(report) << '\n';

    return 0;
}
