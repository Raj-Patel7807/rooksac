# Operations & Deployment

RookSac production infrastructure is organized into isolated containerized tiers managed via Docker Compose under `deploy/docker/`.

## Architecture Topology

```
[ User Browser ]
       |
       v (Port 8080/443: HTTPS, CSP, Security Headers)
[ Frontend (Nginx + React SPA) ]
       |
  (Internal Bridge Network: internal-net)
       |
       +--> [ Backend (FastAPI + C++ UCI Engine Pool) ]
       |          |
       |          +--> [ PostgreSQL 16 (DB Data) ]
       |          +--> [ Redis 7 (Cache & Queue) ]
```

## Runbook & Guides

- [Runbook & Procedures](runbook.md): Deployment, health checks, backup & restore rehearsal, and rollback workflows.
- [Environment Variables Schema](../../deploy/ENV_SCHEMA.md): Complete specification of configuration and secrets.
- [v1.0.0 Release Notes](../product/release-notes-v1.0.md): Product summary, features, and limitations.
