# ADR 026: Frontend Architecture with React, TypeScript, and Vite

## Status
Accepted

## Context
RookSac requires a responsive, accessible client frontend application for real-time play against the autonomous C++20 engine, position analysis, saved games management, and settings configuration. The architecture requires that rules validation and engine move generation remain authoritative on the server/engine, keeping the frontend decoupled as an ergonomic presentation layer.

## Decision
1. **Tooling & Build System**: Implemented using React 19, TypeScript, and Vite for fast builds and modular development under `app/frontend/`.
2. **Component Library & Chessboard**: Integrated `react-chessboard` (MIT licensed) wrapped in a swappable, decoupled `Board` component to permit future board renderer substitutions without leaking library dependencies.
3. **Modular Feature Hierarchy**:
   - `src/components/board/`: Board wrapper and promotion chooser dialog.
   - `src/components/play/`: Play screen with legal move validation, move history, engine thinking indicators, and game lifecycle actions (restart, save).
   - `src/components/analysis/`: Analysis screen featuring FEN loading, dynamic centipawn evaluation bar with sigmoid scaling, and engine telemetry metrics (best move, score, depth, nodes, NPS).
   - `src/components/games/`: Saved games archive and PGN import/export interface.
   - `src/components/settings/`: Search depth, move time, and display configuration.
   - `src/components/ui/`: Reusable primitives (`Button`, `Card`).
   - `src/api/`: Typed API client (`client.ts`), data transfer models (`types.ts`), and resilient WebSocket client (`websocket.ts`) with out-of-order/stale event dropping and reconnection logic.
4. **Security & Secrets**: The client operates strictly through relative `/api/v1` endpoints (proxied in Vite development to `http://127.0.0.1:8000`). No private server URLs, engine binary paths, or credentials exist in frontend code or client bundles.

## Consequences
- Clean separation between presentation and engine logic.
- Responsive layout across desktop and mobile screens.
- Zero secrets or internal paths leaked into production bundles.
- Vitest unit tests verify API client parsing, WebSocket reconnection/sequence filtering, and UI components.
