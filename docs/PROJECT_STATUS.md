# Project status

This page is the authoritative status summary. It describes repository facts, not planned capabilities.

## Implemented

- Repository governance: agent guidance, contribution guidance, security reporting policy, support guidance, formatting/editor settings, ignore rules, and text attributes.
- Documentation structure: indexed product, architecture, engine, testing, engineering, reference, decision, ML, application, operations, and legal sections.
- Repository-layout decision record and lowercase kebab-case documentation naming, except conventional `README.md` indexes.
- Native build foundation: CMake 3.23+, C++20 smoke-test target, CTest registration, Debug/Release/RelWithDebInfo presets, compiler warnings, clang-format and optional clang-tidy/sanitizer targets.
- CI quality gates: Linux GCC 12 and Clang 15 builds, Windows MSVC build, CTest, formatting, clang-tidy, and Clang ASan/UBSan jobs.
- Core chess primitives: strongly typed colors, pieces, files, ranks, squares, bitboards, masks, and compact move encoding with exhaustive primitive tests.

## In progress

- Legal and provenance baseline: the repository inventory and future review process are documented, but licence selection and copyright ownership require owner confirmation.

## Planned

- Position state, FEN parsing, legal move generation, and a standalone C++20 chess engine under `engine/`.
- Perft and benchmark tools, offline research tooling, and repository tools.
- A separate application layer under `app/` that communicates with the engine through UCI/process boundaries.
- CI, packaging, deployment, and release processes.

## Deferred

- Publishing a license, copyright notices, SPDX source headers, and any `NOTICE` file are deferred until the owner confirms the copyright holder, year policy, and license choice.
- Any networked application, database, queue, frontend, deployment, neural evaluation, or Python runtime integration is deferred to later roadmap work.

## Current limitations

There is no position state, FEN parser, move generator, engine executable, perft tool, benchmark tool, UCI command, or playable binary. `rooksac_core` is currently a header-only target containing only foundational value types and bitboard primitives.
