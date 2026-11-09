# ADR 003: Platform, API, and Deployment Architecture

## Status

Accepted (Consolidated from ADR-017, ADR-025, ADR-026, and ADR-027)

## Context

RookSac delivers its engine capabilities through multiple interfaces: standalone UCI command-line operation for standard chess GUIs, an asynchronous backend service managing concurrent engine sessions, a responsive modern web application, and containerized deployment infrastructure.

## Consolidated Decisions

### 1. UCI Protocol & Standalone Executable (Historical ADR-017)

- Engine operates via standard Universal Chess Interface (UCI) protocol (`uci`, `isready`, `position`, `go`, `stop`, `quit`, and `bench`).
- All engine diagnostic information and benchmark telemetry route through dedicated protocol-compliant streams (summary to `stdout`, verbose metrics to `stderr`), preventing GUI desynchronization.

### 2. Scalable Asynchronous Backend & Bounded Engine Pool (Historical ADR-025)

- FastAPI asynchronous backend (`app/backend/`) exposing REST endpoints for health checks, FEN position analysis, and PGN ingestion.
- `BoundedEnginePool`: Enforces strict worker concurrency limits via `asyncio.Semaphore`, preventing CPU starvation.
- Engine lifecycle management: Asynchronous subprocesses with timeout guards, safe stdout line buffering, and guaranteed cleanup on job completion or worker cancellation.

### 3. Modular React TypeScript Frontend (Historical ADR-026)

- Modern SPA built on Vite, React 19, and TypeScript located in `app/frontend/`.
- Modular feature directories: `board/`, `play/`, `analysis/`, `games/`, `settings/`, `ui/`, and `api/`.
- Swappable board layer: Wraps `react-chessboard` behind a clean generic interface to enable drop-in renderer replacements.
- Client/Server authority: The frontend is strictly a presentation and interaction client; rules validation and engine move calculations remain authoritative in the C++ engine.
- Resilient WebSocket Client: Tracks monotonically increasing sequence numbers and drops stale out-of-order engine updates.

### 4. Production Hardening & Multi-Stage Deployment (Historical ADR-027)

- Multi-stage Docker packaging (`Dockerfile.backend` and `Dockerfile.frontend`) eliminating compilers, dev tools, and package managers from runtime images.
- Non-root execution: Backend executes under unprivileged system user `rooksac`; frontend under unprivileged user `nginx`.
- Network tier isolation: PostgreSQL 16 and Redis 7 communicate exclusively across an internal Docker bridge network (`internal-net`) without exposing host ports.
- Web security: Nginx reverse proxy enforces HSTS, CSP, and framing defenses. FastAPI backend applies sliding-window IP rate limiting (120 req/min) and strict CORS origin validation.
- Zero secrets in client bundles: Relative API paths (`/api/v1`) prevent leaking private internal hostnames or credentials.

## Consequences

- Clean end-to-end integration from C++ engine to web interface.
- Protected infrastructure with non-root security boundaries and resource bounds.
- Documented backup, restore, and rollback procedures.
