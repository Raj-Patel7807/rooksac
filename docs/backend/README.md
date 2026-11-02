# Backend architecture and engine orchestration

RookSac provides a modular FastAPI web service located in `app/backend/`. It exposes REST APIs for health monitoring, chess position analysis, and PGN game ingestion while orchestrating isolated, native UCI engine processes safely.

## Key Principles

- **Decoupled Search Execution:** The API never executes CPU-heavy search algorithms directly inside request handlers. All analysis is offloaded to native engine worker processes.
- **Bounded Worker Pool:** `BoundedEnginePool` maintains an `asyncio.Semaphore` capping concurrent active searches to a configurable budget (default: 4 workers). Requests beyond capacity are queued with backpressure protection.
- **Safe Process Management:** Subprocesses are launched via `asyncio.create_subprocess_exec` using direct argument arrays (strictly avoiding shell interpolation). Worker processes are cleanly terminated on shutdown.
- **Safe PGN Ingestion:** Game payloads are capped at 2MB with malformed game detection and non-standard variant rejection.

## Endpoints

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/api/v1/health` | Service health status, version, and active worker count |
| `POST` | `/api/v1/analysis` | Submits FEN for depth-limited or time-limited analysis |
| `POST` | `/api/v1/pgn/import` | Ingests PGN games and validates header and move integrity |

## Running the Backend

```powershell
# Start FastAPI service on port 8000
python -m uvicorn app.backend.main:app --host 0.0.0.0 --port 8000
```
