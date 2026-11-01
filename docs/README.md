# RookSac documentation

This index separates current facts from planned design. Phase 0 contains documentation only; consult the implementation, tests, and build configuration for current behavior when they exist.

## Product direction

| Document | Contents | Read it when |
| --- | --- | --- |
| [Vision](product/VISION.md) | Motivation, philosophy, goals, and non-goals | You want the project’s purpose and boundaries. |
| [PRD](product/PRD.md) | Product requirements and success criteria | You need the intended product scope. |
| [Requirements](product/REQUIREMENTS.md) | Identified, status-tracked requirements | You are planning or reviewing a feature. |
| [Roadmap](product/ROADMAP.md) | Phased delivery plan and acceptance criteria | You need sequencing and phase boundaries. |

## Architecture

| Document | Contents | Read it when |
| --- | --- | --- |
| [Architecture](architecture/ARCHITECTURE.md) | System overview and component relationships | You need the big picture. |
| [Engine design](architecture/ENGINE_DESIGN.md) | Subsystem responsibilities and data flow | You are designing an engine change. |
| [Data flow](architecture/DATA_FLOW.md) | Position-to-best-move processing path | You are tracing interfaces or failures. |
| [Decisions](architecture/DECISIONS.md) | Initial architecture decision records | You need the reasoning behind a choice. |

## Engine guides

| Document | Contents | Read it when |
| --- | --- | --- |
| [Board representation](engine/BOARD_REPRESENTATION.md) | Planned position model and bitboard rationale | Beginning Phase 1. |
| [Move generation](engine/MOVE_GENERATION.md) | Legal move correctness strategy | Beginning Phase 2. |
| [Search](engine/SEARCH.md) | Planned search progression | Working on search phases. |
| [Position evaluation](engine/POSITION_EVALUATION.md) | Classical evaluation direction | Designing evaluation. |
| [Transposition table](engine/TRANSPOSITION_TABLE.md) | Hashing and table design | Adding search caching. |
| [Time management](engine/TIME_MANAGEMENT.md) | Planned clock-aware search | Integrating timed play. |
| [UCI protocol](engine/UCI_PROTOCOL.md) | Planned GUI protocol boundary | Preparing UCI support. |

## Testing and performance

| Document | Contents | Read it when |
| --- | --- | --- |
| [Testing](testing/TESTING.md) | Test strategy and quality gates | Adding or reviewing behavior. |
| [Perft](testing/PERFT.md) | Move-generation validation methodology | Debugging legal moves. |
| [Benchmarking](testing/BENCHMARKING.md) | Reproducible measurement practices | Comparing a change. |
| [Performance](testing/PERFORMANCE.md) | Performance design and profiling principles | Optimizing measured bottlenecks. |

## Engineering and references

| Document | Contents | Read it when |
| --- | --- | --- |
| [Code rules](engineering/CODE_RULES.md) | C++ and Python standards | Writing project code or tooling. |
| [Build](engineering/BUILD.md) | Intended CMake build policy | Introducing a build configuration. |
| [Glossary](reference/GLOSSARY.md) | Shared engine terminology | A term is unfamiliar. |
| [References](reference/REFERENCES.md) | External primary and learning sources | Researching a design. |

Repository-level [contribution guidance](../CONTRIBUTING.md), [security policy](../SECURITY.md), and [agent instructions](../AGENTS.md) also apply.
