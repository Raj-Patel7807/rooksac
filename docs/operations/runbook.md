# Deployment & Operations Runbook

## 1. Production Architecture Overview

The RookSac deployment comprises four isolated service tiers interconnected through a private Docker bridge network:
- **Frontend**: Nginx reverse proxy serving minified React SPA assets and proxying `/api` and `/ws` to the backend.
- **Backend**: FastAPI async service orchestrating the native C++20 UCI engine pool (`rooksac`).
- **Database (db)**: PostgreSQL 16 database storing user profiles and game histories (not publicly exposed).
- **Cache (redis)**: Redis 7 providing rate limiting and async job queue state (not publicly exposed).

## 2. Security Hardening Checklist

- [x] Multi-stage Docker builds without development tooling in runtime layers.
- [x] Non-root execution (`rooksac` user in backend, `nginx` user in frontend).
- [x] Security headers: `Strict-Transport-Security`, `X-Content-Type-Options`, `X-Frame-Options`, `Content-Security-Policy`.
- [x] Origin validation & strict CORS allowlist.
- [x] Per-IP sliding-window rate limiting (120 req/min).
- [x] Private internal networking: PostgreSQL (5432) and Redis (6379) are inaccessible from outside Docker.
- [x] Zero browser secrets or exposed UCI binary paths.

## 3. Deployment Commands

```bash
# Build and run containers in detached mode
docker compose -f deploy/docker/docker-compose.yml up --build -d

# Check service health and logs
docker compose -f deploy/docker/docker-compose.yml ps
docker compose -f deploy/docker/docker-compose.yml logs -f backend
```

## 4. Backup & Restore Procedures

### Database Backup
```bash
docker exec -t rooksac-db pg_dump -U rooksac_user rooksac_db | gzip > backup_$(date +%Y%m%d_%H%M%S).sql.gz
```

### Database Restore Rehearsal
```bash
# Decompress and restore into database
gunzip -c backup_20261102_100000.sql.gz | docker exec -i rooksac-db psql -U rooksac_user -d rooksac_db
```

## 5. Rollback Plan

If an issue arises after deploying an updated image:
```bash
# Revert image tag in docker-compose.yml and restart
docker compose -f deploy/docker/docker-compose.yml down
docker compose -f deploy/docker/docker-compose.yml up -d
```
