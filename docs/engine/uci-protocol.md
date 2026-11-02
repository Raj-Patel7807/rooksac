# UCI protocol

## Concept and value

The Universal Chess Interface (UCI) is a standard text-based protocol that allows chess engines to communicate transparently with graphical interfaces (GUIs), competition drivers, and web platform backends. Implementing UCI decouples RookSac from any particular UI or tournament harness.

## Architecture and Stream Discipline

In RookSac, UCI protocol handling is cleanly decoupled from internal board representation and search:
- **`rooksac::uci::EngineSession`**: Manages the I/O event loop, line tokenization, position setup, and search invocation.
- **`stdout` Discipline**: The standard output stream is strictly reserved for valid UCI protocol output (`id`, `uciok`, `readyok`, `bestmove`, etc.). No debug text, banners, or evaluation dumps pollute `stdout`.
- **`stderr` Diagnostics**: Malformed commands, unparsable FEN strings, and diagnostic notifications are routed to standard error (`stderr`), keeping the GUI communication channel clean.

## Supported Commands

| Command | Syntax / Example | Behavior |
|---|---|---|
| `uci` | `uci` | Identifies the engine (`id name RookSac 0.1.0`, `id author RookSac Authors`) and emits `uciok`. |
| `isready` | `isready` | Synchronizes GUI and engine; responds immediately with `readyok`. |
| `ucinewgame` | `ucinewgame` | Resets the active board state to the starting position and halts active searches. |
| `position startpos` | `position startpos [moves e2e4 e7e5 ...]` | Sets the board to the standard starting position and plays the optional sequence of moves. |
| `position fen` | `position fen <fen_string> [moves ...]` | Initializes the board from a FEN string and applies following legal moves. |
| `go` | `go [depth <N>] [movetime <ms>] [wtime <ms> btime <ms> winc <ms> binc <ms>] [nodes <N>] [infinite]` | Launches search with specified constraints on a dedicated worker thread. |
| `stop` | `stop` | Signals the cooperative search stop token; search aborts gracefully and emits the `bestmove` of the last completed iteration. |
| `quit` | `quit` | Immediately halts search and exits the engine process cleanly. |
| `setoption` | `setoption name <name> [value <val>]` | Defensively handled; unsupported options log an info message to `stderr` without advertising invalid parameters. |

## GUI Integration (Cute Chess, Arena)

RookSac can be configured as an external engine in any standard UCI GUI:
1. **Executable path**: Path to compiled `rooksac` (or `rooksac.exe` on Windows).
2. **Protocol**: Universal Chess Interface (`uci`).
3. **Cute Chess CLI Example**:
   ```bash
   cutechess-cli -engine cmd=./rooksac name=RookSac -engine cmd=./opponent name=Opponent -each tc=40/60+1 -rounds 10
   ```
