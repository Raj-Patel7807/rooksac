# Architecture Decision Records (ADRs)

RookSac architectural decisions are consolidated into three major, comprehensive decision records:

1. [ADR 001: Native Chess Engine Core Architecture](ADR_001_ENGINE_ARCHITECTURE.md)
    - _Historical references consolidated:_ ADR-007 (Repository layout), ADR-008 (Legal provenance), ADR-009 (Bitboard primitives), ADR-010 (Position & reversible state), ADR-011 (Magic bitboards & attack masks), ADR-012 (Legal move generation & perft certification), ADR-013 (Baseline agents & material evaluation).

2. [ADR 002: Search, Evaluation, and Learning Architecture](ADR_002_SEARCH_EVALUATION_AND_LEARNING.md)
    - _Historical references consolidated:_ ADR-014 (Minimax & negamax), ADR-015 (Alpha-beta pruning), ADR-016 (Iterative deepening & quiescence), ADR-018 (Zobrist hashing & transposition table), ADR-019 (Classical tapered evaluation), ADR-020 (Advanced search & benchmarking), ADR-021 (Texel tuning & SPRT), ADR-022 (Supervised policy-value training), ADR-023 (Native neural evaluation), ADR-024 (NNUE research track).

3. [ADR 003: Platform, API, and Deployment Architecture](ADR_003_SYSTEM_AND_DEPLOYMENT_ARCHITECTURE.md)
    - _Historical references consolidated:_ ADR-017 (UCI protocol & standalone binary), ADR-025 (FastAPI backend & engine pool), ADR-026 (React TypeScript frontend), ADR-027 (Production deployment & hardening).
