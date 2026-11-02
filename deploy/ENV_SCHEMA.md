# RookSac Environment Variables Specification

This document defines the environment variables schema for RookSac deployment.

| Variable Name | Description | Status | Classification | Default (Local/Dev) | Production Target |
|:---|:---|:---|:---|:---|:---|
| `ENVIRONMENT` | Runtime environment mode | Optional | Public | `production` | `production` / `staging` |
| `ENGINE_BINARY_PATH` | Path to the native C++ RookSac UCI engine executable | Required | Private | `build/rooksac.exe` | `/opt/rooksac/bin/rooksac` |
| `MAX_CONCURRENT_WORKERS`| Maximum concurrent UCI worker processes | Optional | Private | `4` | `4` - `16` (per CPU cores) |
| `JOB_TIMEOUT_SECONDS` | Maximum search/analysis job timeout in seconds | Optional | Private | `30.0` | `30.0` |
| `MAX_QUEUE_SIZE` | Maximum pending engine analysis requests | Optional | Private | `100` | `100` |
| `DATABASE_URL` | PostgreSQL connection string | Required in Prod | Secret | `sqlite:///./rooksac.db` | `postgresql://user:pass@db:5432/rooksac_db` |
| `REDIS_URL` | Redis cache and queue connection string | Required in Prod | Secret | `redis://localhost:6379/0` | `redis://redis:6379/0` |
| `MAX_REQUESTS_PER_MINUTE`| Sliding window rate limit per client IP | Optional | Private | `120` | `120` |
| `ALLOWED_ORIGINS` | Comma-separated list of permitted CORS origins | Required in Prod | Private | `http://localhost:3000` | `https://rooksac.example.com` |
| `DB_PASSWORD` | PostgreSQL user password | Required in Prod | Secret | (empty in dev) | (Managed Secret Vault) |

### Secret Management Rules
1. Never commit `.env` or files containing live credentials.
2. In production, inject secrets via environment management (Docker secrets, Kubernetes secrets, or cloud vault).
3. The frontend web client contains NO secret variables; all communication routes via relative `/api/v1` and `/ws` endpoints.
