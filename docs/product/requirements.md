# Requirements register

All entries are **Planned** in Phase 0. Statuses are Planned, In Progress, Implemented, Verified, or Deferred. “Implemented” requires code; “Verified” additionally requires recorded relevant validation.

| ID         | Requirement                                                              | Status  |
| ---------- | ------------------------------------------------------------------------ | ------- |
| CORE-001   | The engine must represent a complete standard chess position.            | Planned |
| CORE-002   | The engine must parse and emit supported FEN positions.                  | Planned |
| MOVE-001   | The engine must generate pseudo-legal moves for all pieces.              | Planned |
| MOVE-002   | The engine must generate only legal moves after legality filtering.      | Planned |
| MOVE-003   | The engine must handle promotions, castling, and en passant.             | Planned |
| SEARCH-001 | The engine must search a position and select a best move.                | Planned |
| SEARCH-002 | The engine must use alpha-beta pruning after its basic-search milestone. | Planned |
| EVAL-001   | The engine must evaluate material and planned positional terms.          | Planned |
| HASH-001   | The engine must identify positions for planned transposition-table use.  | Planned |
| TIME-001   | The engine must obey supported search time limits.                       | Planned |
| PROTO-001  | The engine must support its documented UCI command subset.               | Planned |
| TEST-001   | Move generation must be validated with perft reference counts.           | Planned |
| TEST-002   | Implemented subsystems must have proportionate automated tests.          | Planned |
| PERF-001   | Performance comparisons must report reproducible conditions.             | Planned |
