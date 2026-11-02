# ADR-017 — UCI protocol and standalone engine executable

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

To make RookSac universally interoperable with chess graphical interfaces (e.g. Cute Chess, Arena, Banksia, Lichess-bot) and provide a clean boundary for future web application integration (FastAPI backend), the engine requires a standardized text protocol. The Universal Chess Interface (UCI) is the de facto industry standard for computer chess engines. The protocol layer must remain strictly decoupled from internal chess representations and search routines, enforce clean standard I/O streams without stdout pollution, and safely coordinate search cancellation.

## Decision

1. **Protocol Separation:** UCI parsing and session lifecycle management are encapsulated in a dedicated module (`rooksac::uci`, `uci.hpp`, `uci.cpp`). Core engine modules (`Position`, `search`, `time_management`) remain decoupled from protocol details.
2. **Stream Discipline:** The standard output stream (`stdout`) is strictly reserved for valid UCI protocol tokens (`id`, `uciok`, `readyok`, `bestmove`, and future `info` telemetry). All diagnostics, warnings, and error messages are directed to standard error (`stderr`), preventing GUI desynchronization.
3. **Supported Command Subset:**
   - Handshake: `uci`, `isready`.
   - Lifecycle: `ucinewgame`, `quit`.
   - Setup: `position startpos [moves ...]`, `position fen <fen> [moves ...]`.
   - Search: `go` supporting `depth <N>`, `movetime <ms>`, `nodes <N>`, `wtime <ms>`, `btime <ms>`, `winc <ms>`, `binc <ms>`, `movestogo <N>`, and `infinite`.
   - Control: `stop` (triggers cooperative search cancellation and cleanly emits `bestmove`).
   - Configuration: `setoption` (received defensively; no unsupported options are advertised or accepted).
4. **Defensive Parsing:** Input is parsed defensively with bounds checking and tokenization. Malformed commands or illegal moves in position lines log diagnostic notifications to `stderr` without mutating board state or crashing the engine process.
5. **Thread-Safe Search Lifecycle:** Search runs on a dedicated worker thread, coordinated via `SearchStopToken` (`std::atomic<bool>`). Prior to starting any new search or updating the position, active searches are cooperatively signaled to stop and joined cleanly without thread detachment or mid-operation abortion.
6. **Standalone Binary:** The standalone engine executable `rooksac` is compiled from `engine/apps/rooksac_main.cpp`, linking against `rooksac_core`.

## Alternatives considered

- **Custom JSON/REST Protocol for Web:** Rejected for the core executable. UCI provides immediate compatibility with hundreds of existing chess tools, test harnesses, and GUIs. The web platform will wrap or communicate with the engine process via standard UCI.
- **Detached Worker Threads:** Rejected because terminating an engine thread asynchronously risks leaving mutexes locked or half-updated memory states.

## Consequences

RookSac now operates as a fully functional standalone UCI chess engine compatible with Cute Chess and other UCI interfaces.
