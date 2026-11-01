# UCI protocol

## Concept and value

The Universal Chess Interface (UCI) is a text protocol that lets an engine communicate with chess GUIs. It keeps RookSac usable with existing interfaces without embedding a GUI.

## Planned RookSac design

UCI support is a future milestone. The intended command subset includes `uci`, `isready`, `ucinewgame`, `position`, `go`, `stop`, `quit`, and `setoption`. Boundary code will parse commands, configure a search request, and format only protocol output; it will not own chess rules.

## Alternatives and trade-offs

A custom protocol could be smaller but forfeits ecosystem compatibility. UCI parsing is line-oriented but timing and state semantics need careful handling.

## Edge cases and testing

Malformed input, unknown options, repeated commands, a stop during search, FEN plus move lists, and output ordering need transcript-based integration tests. No command is supported in Phase 0.

## Future improvements and references

Options, diagnostics, and non-standard commands need documented scope before adoption. See [official UCI specification](https://www.shredderchess.com/chess-info/features/uci-universal-chess-interface.html) and [data flow](../architecture/DATA_FLOW.md).
