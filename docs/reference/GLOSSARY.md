# Glossary

| Term | Meaning |
| --- | --- |
| Bitboard | A 64-bit value in which each bit represents one board square. |
| FEN | Forsyth–Edwards Notation, a text representation of a chess position. |
| UCI | Universal Chess Interface, a text protocol between engine and GUI. |
| Perft | Recursive legal-move node counting used to validate move generation. |
| Negamax | A symmetric minimax formulation using score negation between players. |
| Alpha-beta pruning | Search pruning that discards branches unable to change a result window. |
| Quiescence search | Selective continuation of unstable positions, often tactical captures. |
| Transposition table | A hash-indexed cache of prior search results. |
| Zobrist hashing | Position hashing by XORing random keys for state features. |
| Iterative deepening | Repeating search at increasing depths, reusing prior knowledge. |
| Move ordering | Trying likely strong moves earlier to improve pruning. |
| Principal variation | The currently best line of play found by search. |
| Evaluation function | A function estimating a non-terminal position’s value. |
| NPS | Nodes per second, a search-throughput metric. |
| Ply | One move by one side; two plies make a full move. |
| Search depth | Number of plies searched from a root or node. |
| Checkmate | A checked side has no legal move. |
| Stalemate | A side not in check has no legal move. |
