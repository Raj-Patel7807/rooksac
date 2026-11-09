# Roadmap

No dates or completion claims are assigned. A phase begins only after its dependencies are sufficiently satisfied and ends when its acceptance criteria and testing needs are met.

| Phase               | Objective and deliverables                           | Dependencies              | Acceptance and testing                                    |
| ------------------- | ---------------------------------------------------- | ------------------------- | --------------------------------------------------------- |
| 0: Foundation       | Documentation, policies, project direction.          | None                      | Links and consistency checked; no engine code introduced. |
| 1: Position         | Position state, square conventions, FEN scope.       | 0                         | Position-state tests and round-trip validation.           |
| 2: Moves            | Pseudo-legal moves, legality, special moves.         | 1                         | Perft references and edge-case tests.                     |
| 3: Basic search     | Negamax and a minimal evaluation baseline.           | 2                         | Deterministic search tests and legal best-move output.    |
| 4: Alpha-beta       | Pruning and move ordering.                           | 3                         | Correctness preserved; measured comparisons.              |
| 5: Deepening and TT | Iteration, hashing, table, basic clocks.             | 4                         | Hash/table tests; time-bound behavior tests.              |
| 6: Evaluation       | Positional terms and phase awareness.                | 5                         | Term-level tests and controlled position suites.          |
| 7: UCI              | UCI command handling and options.                    | 5                         | Protocol transcript/integration tests.                    |
| 8: Performance      | Profiling, benchmark harness, measured improvements. | 5–7 as applicable         | Reproducible benchmark records.                           |
| 9: Advanced search  | PVS, aspiration windows, heuristics when justified.  | 8                         | Regression suite plus before/after evidence.              |
| 10: Python research | Analysis, tuning, and neural experiments.            | Stable engine interfaces  | Reproducible offline experiments; no runtime dependency.  |
| 11: v1.0            | Stable documented release process.                   | Relevant preceding phases | Full validation, reproducibility, and release review.     |

Each phase may refine its detailed design through an ADR before implementation. A later phase does not retroactively make an earlier subsystem “complete” without its own evidence.
