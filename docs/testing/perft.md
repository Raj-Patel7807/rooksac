# Perft

Perft (“performance test”) recursively counts the number of legal move-tree nodes to a chosen depth. It does not measure engine strength; matching trusted counts strongly validates move generation and position-state handling.

## Why it matters

A single wrong legal move can poison every search result. Perft exercises ordinary moves and, with suitable positions, castling, en passant, promotions, checks, and pins. A future `divide` mode should report each root move and its subtree count, making a mismatch easier to localize.

## Implemented Perft engine and verified counts

RookSac includes an authoritative Perft and Divide implementation in `rooksac::perft` with the following certified reference counts (verified against the [Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results)):

### Initial Position (`rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`)

- Depth 1: 20
- Depth 2: 400
- Depth 3: 8,902
- Depth 4: 197,281

### Position 2: Kiwipete (`r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`)

- Depth 1: 48
- Depth 2: 2,039
- Depth 3: 97,862

### Position 3 (`8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`)

- Depth 1: 14
- Depth 2: 191
- Depth 3: 2,812
- Depth 4: 43,238

### Position 4 (`r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1`)

- Depth 1: 6
- Depth 2: 264
- Depth 3: 9,467

### Position 5 (`rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8`)

- Depth 1: 44
- Depth 2: 1,486
- Depth 3: 62,379

The Divide output (`perft::divide`) outputs the node count for each individual root move, allowing rapid localization of any discrepancy during debugging.
