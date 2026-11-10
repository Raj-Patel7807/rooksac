# Production Readiness Checklist

This document details the production-readiness status, verification commands, and evidence for every subsystem in RookSac.

## 1. Automated Testing & Quality Gates

| Gate / Subsystem | Verification Command | Expected Result | Status |
| :--- | :--- | :--- | :--- |
| **C++ Core Engine Tests** | `ctest --test-dir build --output-on-failure` | 15/15 tests pass (0 failures) | ✅ VERIFIED |
| **Perft Regression** | Run target `rooksac_perft_test` | Standard suite matches CPW perft node counts | ✅ VERIFIED |
| **Tactical Puzzle Suite** | Run target `rooksac_tactical_test` | All tactical test positions solved within depth | ✅ VERIFIED |
| **UCI Protocol Conformance** | Run target `rooksac_uci_test` | Handles `uci`, `isready`, `position`, `setoption`, `go`, `stop` | ✅ VERIFIED |
| **FastAPI Backend Tests** | `uv run python -m unittest discover -s app/backend/tests` | 10/10 tests pass (health, analysis, PGN, rate limit) | ✅ VERIFIED |
| **ML & Research Pipeline** | `uv run python -m unittest discover -s research/tests` | 22/22 tests pass (tensor parity, data streaming, training) | ✅ VERIFIED |
| **Deployment Smoke Tests**| `uv run python -m unittest discover -s tools/tests` | 7/7 tests pass (download_model, docker_smoke, sprt) | ✅ VERIFIED |
| **Frontend Lint & Types** | `cd app/frontend && npm run lint && npx tsc -b` | 0 errors | ✅ VERIFIED |
| **Frontend Unit Tests**   | `cd app/frontend && npm test` | 19/19 Vitest tests pass | ✅ VERIFIED |
| **Frontend Prod Build**   | `cd app/frontend && npm run build` | Builds bundle without error into `dist/` | ✅ VERIFIED |

## 2. Security & Process Isolation

- [x] **Subprocess Safety**: All engine processes are spawned using direct argument lists (`asyncio.create_subprocess_exec` / `subprocess.Popen`) with `shell=False`.
- [x] **No Secret Leaks**: Verified by CI scan: no `.env`, private keys (`BEGIN RSA PRIVATE KEY`), or Hugging Face tokens (`hf_*`) committed to Git.
- [x] **Strict Input Validation**:
  - All incoming FENs validated with `chess.Board(fen)` prior to engine submission.
  - Analysis depths strictly capped to `[1, MAX_ANALYSIS_DEPTH]`.
  - PGN uploads enforced with maximum byte ceiling (`MAX_PGN_FILE_SIZE_BYTES = 2MB`).
- [x] **CORS & WebSocket Origin Validation**:
  - HTTP CORS middleware validates `ALLOWED_ORIGINS`.
  - WebSocket connection requests immediately close with code `1008` (Policy Violation) if origin header does not match `ALLOWED_ORIGINS`.
- [x] **Rate Limiting & Proxy Headers**: Sliding-window IP limiter parses `X-Forwarded-For` only when the direct peer IP belongs to `TRUSTED_PROXIES`.
- [x] **Non-Root Execution**:
  - Backend Docker runs as dedicated system user `rooksac` (UID 10001).
  - Frontend Docker runs as `nginx`.
- [x] **Safe Structured Logging**: Logs emit structured JSON without logging passwords, secrets, or raw PGN dumps.

## 3. Container & Deployment Architecture

- [x] **Builder / Runtime Compatibility**: Backend multi-stage Docker build uses `python:3.12.9-slim-bookworm` across builder and runtime stages, guaranteeing consistent GCC 12 / `libstdc++` runtime ABI.
- [x] **Dynamic Port Binding**: FastAPI binds to `0.0.0.0:${PORT:-8000}`, adapting automatically to Render's dynamic port assignment.
- [x] **Single Uvicorn Worker**: Governs engine lifecycle within a single Python process pool (`BoundedEnginePool`), avoiding runaway process duplication.
- [x] **Readiness Docker Healthcheck**: Docker container health check probes `/api/v1/health`. Returns 200 OK only when active UCI workers report `readyok`.
- [x] **Model Integrity Verification**:
  - The native `.rks` format embeds SHA-256 header checksums verified before weight memory allocation.
  - Exporter (`tools/export_model.py`) and Downloader (`tools/download_model.py`) verify SHA-256 digests.
- [x] **Vercel Decoupling**: Frontend is fully decoupled via `src/config/env.ts` (`VITE_API_BASE_URL` and `VITE_WS_BASE_URL`) and includes `vercel.json` SPA routing.

## 4. Rollback Runbook

### Service Rollback (Render / Docker)
If a newly deployed container fails health checks or exhibits regressions:
1. Re-deploy the previous pinned image tag or Git commit in the Render Dashboard (Deploy > Rollback to commit `<previous-sha>`).
2. In Docker Compose environments:
   ```bash
   docker compose -f deploy/docker/docker-compose.yml down
   # Roll back code checkout to previous release tag
   git checkout tags/v1.0.0
   docker compose -f deploy/docker/docker-compose.yml up --build -d
   ```

### Neural Model Rollback
If a newly trained model exhibits tactical or positional regression:
1. Set `ENGINE_EVAL_MODE=Classical` in the environment to immediately revert to the deterministic classical evaluation engine without redeploying code.
2. Alternatively, switch `ENGINE_MODEL_PATH` to the previous verified model path:
   ```bash
   # Update environment variable in Render or Docker Compose
   ENGINE_MODEL_PATH=/opt/rooksac/models/model_v1.rks
   ```
3. Restart or trigger container reload to spawn engine workers with the restored model.
