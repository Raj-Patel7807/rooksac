# Perft

Perft (“performance test”) recursively counts the number of legal move-tree nodes to a chosen depth. It does not measure engine strength; matching trusted counts strongly validates move generation and position-state handling.

## Why it matters

A single wrong legal move can poison every search result. Perft exercises ordinary moves and, with suitable positions, castling, en passant, promotions, checks, and pins. A future `divide` mode should report each root move and its subtree count, making a mismatch easier to localize.

## Future regression practice

Use established reference positions and retain cases that exposed defects. Record the exact FEN, depth, expected count, source, and whether counts are legal-move counts. Never publish an RookSac count until it has actually been computed by RookSac.

## Reference positions

Published suites, including the Chess Programming Wiki perft collection, provide known positions covering special rules. They are input references, not results for this repository. See [Perft Results](https://www.chessprogramming.org/Perft_Results) and [Move generation](../engine/MOVE_GENERATION.md).
