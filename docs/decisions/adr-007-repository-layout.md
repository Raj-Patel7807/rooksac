# ADR-007 — Modular monorepo repository layout

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

RookSac begins with documentation only but is intended to grow into a native engine, offline research tooling, and an optional web product. A flat repository would blur build, runtime, and ownership boundaries as those areas arrive. Creating empty implementation trees now would imply work that does not exist.

## Decision

Use a modular monorepo. The future native engine is rooted at `engine/` with `include/rooksac/`, `src/`, and `tests/`. Offline research belongs in `research/`; the optional web product belongs in `app/backend/` and `app/frontend/`; benchmarks, tooling, and deployment material have separate roots. Documentation records each boundary before implementation directories are needed.

The engine must compile and run without Python, Node, a database, Redis, or network access. Research tooling must not be linked into the engine hot path. The application communicates with the engine through UCI/process boundaries and does not reach into engine internals.

## Alternatives considered

- Keep a flat root until implementation begins.
- Create all future directories immediately.
- Split each component into separate repositories or services now.

## Consequences

The layout is discoverable without speculative files. Future changes must preserve independent engine build and test boundaries. More repository roots add navigation overhead, which the documentation index and component indexes address. A distributed deployment architecture is not implied by this decision.
