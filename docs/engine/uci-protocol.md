# UCI protocol

## Concept and value

The Universal Chess Interface (UCI) is a standard text-based protocol that allows chess engines to communicate transparently with graphical interfaces (GUIs), competition drivers, and web platform backends. Implementing UCI decouples RookSac from any particular UI or tournament harness.

## Architecture and Stream Discipline

In RookSac, UCI protocol handling is cleanly decoupled from internal board representation and search:

- **`rooksac::uci::EngineSession`**: Manages the I/O event loop, line tokenization, position setup, and search invocation.
- **`stdout` Discipline**: The standard output stream is strictly reserved for valid UCI protocol output (`id`, `uciok`, `readyok`, `bestmove`, etc.). No debug text, banners, or evaluation dumps pollute `stdout`.
- **`stderr` Diagnostics**: Malformed commands, unparsable FEN strings, and diagnostic notifications are routed to standard error (`stderr`), keeping the GUI communication channel clean.

## Supported Commands

| Command             | Syntax / Example                                                                                    | Behavior                                                                                                                      |
| ------------------- | --------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `uci`               | `uci`                                                                                               | Identifies the engine (`id name RookSac 0.1.0`, `id author RookSac Authors`), advertises options, and emits `uciok`.          |
| `isready`           | `isready`                                                                                           | Synchronizes GUI and engine; emits diagnostic info strings on loaded model/fallback status, and responds with `readyok`.       |
| `ucinewgame`        | `ucinewgame`                                                                                        | Resets the active board state to the starting position and halts active searches.                                             |
| `position startpos` | `position startpos [moves e2e4 e7e5 ...]`                                                           | Sets the board to the standard starting position and plays the optional sequence of moves.                                    |
| `position fen`      | `position fen <fen_string> [moves ...]`                                                             | Initializes the board from a FEN string and applies following legal moves.                                                    |
| `go`                | `go [depth <N>] [movetime <ms>] [wtime <ms> btime <ms> winc <ms> binc <ms>] [nodes <N>] [infinite]` | Launches search with specified constraints on a dedicated worker thread.                                                      |
| `stop`              | `stop`                                                                                              | Signals the cooperative search stop token; search aborts gracefully and emits the `bestmove` of the last completed iteration. |
| `quit`              | `quit`                                                                                              | Immediately halts search and exits the engine process cleanly.                                                                |
| `setoption`         | `setoption name <name> [value <val>]`                                                               | Configures engine runtime parameters (Hash, EvalMode, ModelPath, UsePolicy, NeuralWeight).                                     |

## Supported UCI Options

- **`Hash`** (`type spin`, default: 64, min: 1, max: 1024): Transposition table size in megabytes.
- **`EvalMode`** (`type combo`, default: `Classical`, vars: `Classical`, `Neural`, `Hybrid`):
  - `Classical`: Tapered hand-crafted evaluation (material, PSTs, pawn structure, king safety, mobility).
  - `Neural`: Genuine neural network value prediction loaded from `.rks` format. Strict error if model is invalid.
  - `Hybrid`: Weighted linear combination of classical and neural evaluation (`NeuralWeight` parameter). Falls back safely to classical if no model is loaded.
- **`ModelPath`** (`type string`, default: `<empty>`): Absolute or relative filesystem path to a `.rks` binary model.
- **`UsePolicy`** (`type check`, default: `false`): When enabled, uses neural policy logits to refine move ordering among moves of the same tactical tier.
- **`NeuralWeight`** (`type spin`, default: 50, min: 0, max: 100): Percentage weight given to neural evaluation in Hybrid mode.

## GUI Integration (Cute Chess, Arena)

RookSac can be configured as an external engine in any standard UCI GUI:

1. **Executable path**: Path to compiled `rooksac` (or `rooksac.exe` on Windows).
2. **Protocol**: Universal Chess Interface (`uci`).
3. **Cute Chess CLI Example**:
    ```bash
    cutechess-cli -engine cmd=./rooksac name=RookSac -engine cmd=./opponent name=Opponent -each tc=40/60+1 -rounds 10
    ```
