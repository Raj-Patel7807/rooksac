# Deployment & Operations Runbook

## 1. Production Architecture Overview

RookSac supports two deployment architectures:
1. **Decoupled Cloud Architecture (Recommended)**:
   - **Frontend**: Hosted on [Vercel](https://vercel.com) with client-side SPA routing (`vercel.json`) and edge asset distribution.
   - **Backend**: Hosted on [Render](https://render.com) (or similar container platforms) running FastAPI with the native C++20 UCI engine pool and embedded neural model.
2. **Unified Local / Self-Hosted Stack**:
   - Orchestrated via `deploy/docker/docker-compose.yml`.
   - **Frontend**: Nginx 1.27 serving the SPA bundle and reverse proxying `/api` and `/ws` to the backend.
   - **Backend**: FastAPI async service orchestrating the native C++20 UCI engine (`rooksac`).

## 2. Security Hardening Checklist

- [x] Multi-stage Docker builds without compilers or development dependencies in runtime images.
- [x] Strictly pinned base images (`python:3.12.9-slim-bookworm`, `node:22.14.0-alpine3.21`, `nginx:1.27.4-alpine3.21-slim`).
- [x] Builder and runtime environments share Debian 12 Bookworm for exact `libstdc++` ABI compatibility.
- [x] Non-root execution (`rooksac` UID/GID 10001 in backend, `nginx` in frontend).
- [x] Security headers: `X-Content-Type-Options: nosniff`, `X-Frame-Options: DENY`, `Referrer-Policy: strict-origin-when-cross-origin`.
- [x] Origin validation & strict CORS / WebSocket allowlists (`ALLOWED_ORIGINS`).
- [x] Sliding-window IP rate limiting with trusted proxy header verification.
- [x] Zero browser secrets: no private tokens or API keys exposed in frontend bundles.
- [x] Docker healthcheck verifies full readiness: HTTP 200 returned only after active UCI engine worker responds to `uci`/`isready`.
- [x] Single Uvicorn worker process to govern concurrency within the bounded engine process pool.

## 3. Docker Commands

### Building Backend Image
```bash
# Build production backend image
docker build -t rooksac-backend:latest -f deploy/docker/Dockerfile.backend .
```

### Running Backend Container (Standalone)
```bash
# Run in classical mode on port 8000
docker run -d --name rooksac-backend \
  -p 8000:8000 \
  -e PORT=8000 \
  -e ENGINE_EVAL_MODE=Classical \
  -e ALLOWED_ORIGINS="https://rooksac.vercel.app,http://localhost:3000" \
  rooksac-backend:latest

# Run in neural evaluation mode
docker run -d --name rooksac-backend-neural \
  -p 8000:8000 \
  -e PORT=8000 \
  -e ENGINE_EVAL_MODE=Neural \
  -e ENGINE_MODEL_PATH=/opt/rooksac/models/model_test.rks \
  -e ALLOWED_ORIGINS="https://rooksac.vercel.app" \
  rooksac-backend:latest
```

### Building Frontend Image
```bash
docker build -t rooksac-frontend:latest -f deploy/docker/Dockerfile.frontend .
```

### Local Multi-Container Stack (Docker Compose)
```bash
# Start backend and frontend in detached mode
docker compose -f deploy/docker/docker-compose.yml up --build -d

# Verify health and running containers
docker compose -f deploy/docker/docker-compose.yml ps

# Inspect logs
docker compose -f deploy/docker/docker-compose.yml logs -f backend
```

## 4. Model Artifact Distribution (Hugging Face)

To download and verify a pinned neural model artifact from Hugging Face Hub during staging or deployment:

```bash
python tools/download_model.py \
  --repo-id "org/rooksac-models" \
  --filename "model_v2.rks" \
  --revision "40_character_git_commit_sha" \
  --expected-sha256 "expected_hex_digest" \
  --output "/opt/rooksac/models/model_v2.rks"
```

> [!IMPORTANT]
> If accessing a private repository, supply `HF_TOKEN` as a server-side environment variable. Never embed Hugging Face tokens in Dockerfiles, Git repositories, or frontend bundles.

## 5. Health & Readiness Verification

Probing the readiness endpoint:
```bash
curl -f http://localhost:8000/api/v1/health
```

Expected response in Neural mode:
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "engine_ready": true,
  "model_ready": true,
  "eval_mode": "Neural",
  "fallback_active": false,
  "model_details": "RookSac-NativeCNN 2.0.0 (2 blocks, 32 channels, 4240 actions)",
  "active_workers": 0,
  "queue_size": 0,
  "max_workers": 4
}
```

The container healthcheck automatically marks the container healthy only when `status` is `healthy` and `engine_ready` is `true`.
