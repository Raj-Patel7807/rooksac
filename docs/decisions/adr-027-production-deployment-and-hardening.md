# ADR 027: Production Deployment Topology and Hardening

## Status
Accepted

## Context
RookSac requires a verifiable, secure production deployment for the native C++ engine, Python FastAPI backend, and React TypeScript frontend. The system must prevent unauthorized access to backend worker processes and persistence tiers, mitigate DDoS/abuse risks, enforce HTTPS/CORS standards, and establish repeatable backup and rollback workflows.

## Decision
1. **Container Packaging**: Multi-stage Dockerfiles (`Dockerfile.backend` and `Dockerfile.frontend`) ensure compiler tools, CMake, and build artifacts are excluded from runtime images.
2. **Non-Root Execution**: Backend runs under unprivileged user `rooksac`; frontend runs under unprivileged user `nginx`.
3. **Network Tier Isolation**:
   - Only the Nginx frontend reverse proxy (port 8080/443) is exposed to external traffic.
   - FastAPI backend (port 8000), PostgreSQL (port 5432), and Redis (port 6379) communicate strictly through an internal Docker bridge network (`internal-net`).
4. **Application Hardening**:
   - HTTP security headers (`Strict-Transport-Security`, `X-Content-Type-Options: nosniff`, `X-Frame-Options: DENY`, `Content-Security-Policy`).
   - Sliding-window in-memory rate limiting (120 req/min/IP).
   - Strict CORS origin allowlisting.
5. **Durable Storage & Backups**:
   - Standardized `pg_dump` backup and restore scripts documented in the operations runbook.

## Consequences
- Engine and databases are shielded from direct public network exposure.
- Predictable and auditable deployment via Docker Compose.
- Zero credentials committed into repository or client bundles.
