#include <cstdlib>
#include <iostream>
#include <string>

#include "rooksac/benchmark.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testCanonicalSuiteMetadata() {
    const auto suite = bench::canonicalSuite();
    require(!suite.empty(), "Suite must not be empty");
    require(suite.size() == 8, "Suite must contain exactly 8 standard benchmark positions");

    for (const auto& pos : suite) {
        require(!pos.id.empty(), "Position ID must not be empty");
        require(!pos.name.empty(), "Position name must not be empty");
        require(!pos.fen.empty(), "Position FEN must not be empty");
        require(!pos.theme.empty(), "Position theme must not be empty");
        require(!pos.provenance.empty(), "Position provenance must not be empty");
        require(pos.depth > 0, "Position depth must be positive");
    }
}

void testRunBenchmarkAndRepetitions() {
    // Run a small 2-position subset at depth 2 with 2 repetitions
    std::vector<bench::BenchmarkPosition> subSuite;
    const auto suite = bench::canonicalSuite();
    subSuite.push_back(suite[0]); // Startpos
    subSuite.push_back(suite[5]); // Knight-Pawn

    const auto report = bench::runBenchmark(subSuite, 2, 2);
    require(report.repetitions == 2, "Report repetitions must match requested count");
    require(report.results.size() == 2, "Report result count must match input positions");
    require(report.totalNodes > 0, "Total nodes must be positive");
    require(!report.compiler.empty(), "Compiler name must be populated");
    require(!report.buildType.empty(), "Build type must be populated");
    require(!report.os.empty(), "OS must be populated");
    require(!report.cpu.empty(), "CPU must be populated");

    for (const auto& r : report.results) {
        require(!r.id.empty(), "Result ID must not be empty");
        require(!r.name.empty(), "Result name must not be empty");
        require(r.depth == 2, "Result depth must match requested depth");
        require(r.nodes > 0, "Result nodes must be positive");
        require(!r.bestMove.empty(), "Result bestMove must not be empty");
    }
}

void testReportFormatting() {
    std::vector<bench::BenchmarkPosition> subSuite;
    const auto suite = bench::canonicalSuite();
    subSuite.push_back(suite[0]);

    const auto report = bench::runBenchmark(subSuite, 2, 1);
    const std::string text = bench::formatReport(report);
    require(text.find("RookSac Benchmark Suite Report") != std::string::npos, "Text report header present");
    require(text.find("Startpos") != std::string::npos, "Text report contains position name");

    const std::string json = bench::formatJson(report);
    require(json.find("\"compiler\"") != std::string::npos, "JSON contains compiler field");
    require(json.find("\"total_nodes\"") != std::string::npos, "JSON contains total_nodes field");
    require(json.find("\"positions\"") != std::string::npos, "JSON contains positions array");
    require(json.find("\"pos01_startpos\"") != std::string::npos, "JSON contains position ID");

    const std::string csv = bench::formatCsv(report);
    require(csv.find("id,name,depth,nodes,qnodes,time_ms") != std::string::npos, "CSV contains header row");
    require(csv.find("pos01_startpos,Startpos,2") != std::string::npos, "CSV contains position row");
}

}  // namespace

int main() {
    testCanonicalSuiteMetadata();
    testRunBenchmarkAndRepetitions();
    testReportFormatting();

    std::cout << "All benchmark tests passed successfully.\n";
    return EXIT_SUCCESS;
}

