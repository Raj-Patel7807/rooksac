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
- `src/config/`: Environment configuration (`env.ts`) handling typed API/WS endpoint resolution and protocol upgrades (`http` -> `ws`, `https` -> `wss`).
- `src/api/`: Typed REST API client (`client.ts`), data models (`types.ts`), and WebSocket client with sequence tracking and reconnect handling (`websocket.ts`).

## Configuration & Deployment (Vercel + Render)

The frontend is fully decoupled and can be deployed independently to [Vercel](https://vercel.com) while connecting to a FastAPI backend hosted on [Render](https://render.com) or a self-hosted server.

### Environment Variables

Configure the following variables in `.env.local` for local development overrides or in your Vercel Project Dashboard:

| Variable | Description | Default (Dev / Unset) |
| :--- | :--- | :--- |
| `VITE_API_BASE_URL` | Base URL for FastAPI REST endpoints (e.g. `https://rooksac-api.onrender.com/api/v1`) | `/api/v1` (routes via Vite proxy) |
| `VITE_WS_BASE_URL` | Base URL for FastAPI WebSocket telemetry (e.g. `wss://rooksac-api.onrender.com/ws`) | Derived from `VITE_API_BASE_URL` or window location |

> [!CAUTION]
> Never put API keys, passwords, or secrets into `VITE_*` environment variables. All `VITE_*` variables are embedded directly into the public client-side JavaScript bundle.

### Vercel Deployment

A `vercel.json` configuration file is included in `app/frontend/`:
- Configures client-side SPA routing (`/(.*) -> /index.html`).
- Adds strict security headers (`X-Content-Type-Options`, `X-Frame-Options`, `Referrer-Policy`).
- Implements immutable caching for static assets in `/assets/*`.

For traditional containerized environments, the Docker + Nginx reverse proxy configuration (`deploy/docker/nginx.conf`) remains fully supported by leaving `VITE_API_BASE_URL` unset, routing requests through the same-origin `/api` and `/ws` endpoints.

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
