# Transposition tables and hashing

## Concept and value

Different move orders can reach the same position; such repetitions are transpositions. A transposition table caches search results by a position key to avoid repeating work. Zobrist hashing typically XORs random keys for piece-square, side, castling, and en-passant state into a compact key.

## Planned RookSac design

Future entries are expected to store a key signature, searched depth, score, best-move hint, and bound type: **exact**, **lower bound**, or **upper bound**. Exact values are fully known for the window; lower and upper bounds constrain it. Replacement policy, cluster layout, and score normalization remain implementation decisions to benchmark.

## Alternatives and trade-offs

Tables save nodes but use memory and add probes to hot code. Full keys reduce collision risk but use more space; partial keys save space but make verification crucial. Replacement can favor depth, age, or always-replace; no one policy is assumed before testing.

## Edge cases and testing

Hash collisions are possible and must not become silent correctness assumptions. Keys must distinguish all state relevant to legal moves. Test deterministic key updates, make/unmake restoration, entry bounds, replacement behavior, and memory limits.

## Future improvements and references

Table sizing, generations, and parallel-search concerns are deferred. See [search](search.md) and [Transposition Table](https://www.chessprogramming.org/Transposition_Table).
