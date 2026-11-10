# RookSac Environment Variables Specification

This document defines the environment variables schema for RookSac deployment.

| Variable Name             | Description                                              | Status           | Classification | Default (Local/Dev)                     | Production Target (Render / Container)         |
| :------------------------ | :------------------------------------------------------- | :--------------- | :------------- | :-------------------------------------- | :--------------------------------------------- |
| `ENVIRONMENT`             | Runtime environment mode (`production` / `development`)  | Optional         | Public         | `production`                            | `production`                                   |
| `PORT`                    | HTTP port for FastAPI to listen on                       | Optional         | Public         | `8000`                                  | Set dynamically by Render or cloud host        |
| `ENGINE_BINARY_PATH`      | Path to the native C++ RookSac UCI engine executable     | Required         | Private        | `build/rooksac.exe`                     | `/opt/rooksac/bin/rooksac`                     |
| `ENGINE_EVAL_MODE`        | Evaluation mode: `Classical`, `Neural`, or `Hybrid`      | Optional         | Private        | `Classical`                             | `Neural`                                       |
| `ENGINE_MODEL_PATH`       | Filesystem path to the compiled `.rks` neural model      | Required for NN  | Private        | `research/checkpoints/model_test.rks`   | `/opt/rooksac/models/model_test.rks`           |
| `ENGINE_USE_POLICY`       | Enable neural policy move ordering prior to alpha-beta   | Optional         | Private        | `false`                                 | `true` or `false`                              |
| `ENGINE_NEURAL_WEIGHT`    | Hybrid eval weight (0-100) blending NN and classical     | Optional         | Private        | `50`                                    | `50`                                           |
| `ENGINE_HASH_MB`          | UCI transposition table size in megabytes                | Optional         | Private        | `64`                                    | `64` - `256`                                   |
| `MAX_CONCURRENT_WORKERS`  | Maximum concurrent UCI worker processes in bounded pool  | Optional         | Private        | `4`                                     | `4` - `8` (matching host CPU cores)            |
| `JOB_TIMEOUT_SECONDS`     | Maximum search/analysis job timeout in seconds           | Optional         | Private        | `30.0`                                  | `30.0`                                         |
| `MAX_QUEUE_SIZE`          | Maximum pending engine analysis requests before HTTP 429 | Optional         | Private        | `100`                                   | `100`                                          |
| `MAX_REQUESTS_PER_MINUTE` | Sliding window rate limit per client IP                  | Optional         | Private        | `120`                                   | `120`                                          |
| `ALLOWED_ORIGINS`         | Comma-separated list of permitted CORS/WS origins        | Required in Prod | Private        | `http://localhost:3000`                 | `https://rooksac.vercel.app`                   |
| `TRUSTED_PROXIES`         | Comma-separated list of reverse proxy IPs (for client IP)| Optional         | Private        | `127.0.0.1,::1`                         | `10.0.0.0/8,172.16.0.0/12`                     |

### Frontend Environment Variables (Vercel)

| Variable Name       | Description                                              | Status           | Classification | Production Example                     |
| :------------------ | :------------------------------------------------------- | :--------------- | :------------- | :------------------------------------- |
| `VITE_API_BASE_URL` | Base URL for backend REST API endpoints                  | Required in Prod | Public         | `https://rooksac-api.onrender.com/api/v1` |
| `VITE_WS_BASE_URL`  | Base URL for backend WebSocket telemetry                 | Optional         | Public         | `wss://rooksac-api.onrender.com/ws`   |

### Secret Management Rules

1. Never commit `.env` or files containing live credentials.
2. In production, inject secrets via environment management (Render Environment Dashboard or Vercel Settings).
3. The frontend web client contains NO secret variables; all `VITE_*` variables are embedded into public client JavaScript.
4. The backend minimal image does not require external database or Redis services unless explicitly integrated for extended analytics.
