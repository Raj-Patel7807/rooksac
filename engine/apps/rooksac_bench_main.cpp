#include <fstream>
#include <iostream>
#include <string>

#include "rooksac/benchmark.hpp"

int main(int argc, char* argv[]) {
    std::uint32_t depth = 4;
    std::uint32_t repetitions = 1;
    std::string jsonPath;
    std::string csvPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--depth" && i + 1 < argc) {
            depth = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--reps" && i + 1 < argc) {
            repetitions = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--json" && i + 1 < argc) {
            jsonPath = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            csvPath = argv[++i];
        }
    }

    std::cout << "Running RookSac benchmark (depth=" << depth
              << ", reps=" << repetitions << ")...\n";

    const auto report = rooksac::bench::runBenchmark({}, depth, repetitions);
    std::cout << rooksac::bench::formatReport(report) << '\n';

    if (!jsonPath.empty()) {
        std::ofstream jsonFile(jsonPath);
        if (jsonFile.is_open()) {
            jsonFile << rooksac::bench::formatJson(report);
            std::cout << "Saved benchmark JSON report to: " << jsonPath << '\n';
        } else {
            std::cerr << "Failed to open output file: " << jsonPath << '\n';
        }
    }

    if (!csvPath.empty()) {
        std::ofstream csvFile(csvPath);
        if (csvFile.is_open()) {
            csvFile << rooksac::bench::formatCsv(report);
            std::cout << "Saved benchmark CSV report to: " << csvPath << '\n';
        } else {
            std::cerr << "Failed to open output file: " << csvPath << '\n';
        }
    }

    return 0;
}
