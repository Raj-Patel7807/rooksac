# Frontend Application

The RookSac web frontend is a React + TypeScript single-page application built with Vite, located in `app/frontend/`.

## Architecture

The frontend is strictly a presentation and interaction layer; chess rules, search algorithms, and move selection remain authoritative in the native C++20 engine exposed via the FastAPI backend (`app/backend/`).

### Directory Structure

- `src/components/board/`: Modular chessboard wrapper (`Board.tsx`) and pawn promotion chooser (`PromotionDialog.tsx`).
- `src/components/play/`: Play screen (`PlayScreen.tsx`) managing moves against the engine and move history (`MoveList.tsx`).
- `src/components/analysis/`: Analysis screen (`AnalysisScreen.tsx`), engine telemetry panel (`AnalysisPanel.tsx`), and centipawn win-probability eval bar (`EvalBar.tsx`).
- `src/components/games/`: Saved games archive and PGN import/export interface (`GamesScreen.tsx`).
- `src/components/settings/`: Engine search parameters and user preferences (`SettingsScreen.tsx`).
- `src/components/ui/`: Reusable accessible UI primitives (`Button.tsx`, `Card.tsx`).
- `src/api/`: Typed REST API client (`client.ts`), data models (`types.ts`), and WebSocket client with sequence tracking and reconnect handling (`websocket.ts`).

## Development & Testing

From `app/frontend/`:

```bash
# Install dependencies
npm install

# Start development server (proxies /api to backend at http://127.0.0.1:8000)
npm run dev

# Run Vitest unit & component test suite
npm test

# Build production distribution
npm run build
```
