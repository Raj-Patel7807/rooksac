# ADR-025 — Modular FastAPI backend and bounded UCI engine pool

- **Status:** Accepted
- **Context / problem:** Providing a web API for chess position analysis and game management requires decoupling web request handling from CPU-intensive search. Executing search directly inside web request loops causes resource starvation, unmanaged thread explosion, and process lockups. We need a modular backend architecture supporting 10–100 concurrent players with explicit worker pool bounds.
- **Options considered:**
  1. Microservices architecture with separate services for authentication, game state, and UCI workers.
  2. Running chess search threads directly within ASGI request handlers.
  3. A modular monolith backend built in FastAPI coordinating isolated external UCI engine processes (`rooksac`) through a strictly bounded worker pool (`BoundedEnginePool`).
- **Decision:** Implement option 3:
  - **Modular Architecture:** Layered Python backend structured into `config`, `schemas`, `routes`, `engine_manager`, and tests under `app/backend/`.
  - **Bounded Engine Worker Pool:** `BoundedEnginePool` maintains an `asyncio.Semaphore` enforcing an upper limit on concurrent active search processes (`MAX_CONCURRENT_WORKERS=4`). Excess requests are queued or backpressured.
  - **Isolated Subprocess Lifecycles:** Each analysis job launches or reuses a dedicated UCI engine process via direct argument arrays (`asyncio.create_subprocess_exec`) with zero shell interpolation. Stdio communication adheres strictly to the UCI protocol (`position fen`, `go depth`, `stop`, `quit`).
  - **Safe PGN Parsing:** Ingests PGN games through `parse_game` with explicit file size quotas (`MAX_PGN_FILE_SIZE_BYTES=2MB`) and variant filtering.
- **Consequences:**
  - Guaranteed CPU and memory caps preventing search-induced denial of service.
  - Safe subprocess reap cycles with zero orphaned engine processes.
  - Clear, versioned REST API (`/api/v1/health`, `/api/v1/analysis`, `/api/v1/pgn/import`).
