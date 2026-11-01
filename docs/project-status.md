# Project status

This page is the authoritative status summary. It describes repository facts, not planned capabilities.

## Implemented

- Repository governance: agent guidance, contribution guidance, security reporting policy, support guidance, formatting/editor settings, ignore rules, and text attributes.
- Documentation structure: indexed product, architecture, engine, testing, engineering, reference, decision, ML, application, operations, and legal sections.
- Repository-layout decision record and lowercase kebab-case documentation naming, except conventional `README.md` indexes.

## In progress

Nothing. Sprint 01 establishes the repository baseline only.

## Planned

- A standalone C++20/CMake engine under `engine/`.
- Engine tests, benchmarks, offline research tooling, and repository tools.
- A separate application layer under `app/` that communicates with the engine through UCI/process boundaries.
- CI, packaging, deployment, and release processes.

## Deferred

- License selection, copyright ownership, and third-party provenance are intentionally deferred to Sprint 02.
- Any networked application, database, queue, frontend, deployment, neural evaluation, or Python runtime integration is deferred to later roadmap work.

## Current limitations

There are no source files, build targets, test suites, scripts, workflows, benchmarks, UCI commands, or playable binary. The planned layout is intentionally documented without creating empty implementation directories.
