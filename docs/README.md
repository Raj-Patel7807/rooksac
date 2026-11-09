# RookSac documentation

This index separates current facts from planned design. The repository has CMake build infrastructure, core chess value types, bitboard primitives, position state representation, FEN parsing, reversible make/unmake execution, piece attack tables, pseudo-legal and legal move generation, perft certification, material evaluation, baseline agents (random and greedy), and a match harness, but no tree search or positional evaluation; implemented artifacts define current behavior. Documentation below is authoritative for project intent, governance, and documented decisions.

## Product direction

| Document                                                | Contents                                             | Read it when                                   |
| ------------------------------------------------------- | ---------------------------------------------------- | ---------------------------------------------- |
| [Vision](product/vision.md)                             | Motivation, philosophy, goals, and non-goals         | You want the project’s purpose and boundaries. |
| [Product requirements](product/product-requirements.md) | Product requirements and success criteria            | You need the intended product scope.           |
| [Requirements](product/requirements.md)                 | Identified, status-tracked requirements              | You are planning or reviewing a feature.       |
| [Roadmap](product/roadmap.md)                           | Phased delivery plan and acceptance criteria         | You need sequencing and phase boundaries.      |
| [Project status](PROJECT_STATUS.md)                     | Implemented, in-progress, planned, and deferred work | You need a factual delivery snapshot.          |

## Architecture

| Document                                       | Contents                                    | Read it when                            |
| ---------------------------------------------- | ------------------------------------------- | --------------------------------------- |
| [Architecture](architecture/architecture.md)   | System overview and component relationships | You need the big picture.               |
| [Engine design](architecture/engine-design.md) | Subsystem responsibilities and data flow    | You are designing an engine change.     |
| [Data flow](architecture/data-flow.md)         | Position-to-best-move processing path       | You are tracing interfaces or failures. |
| [Decisions](decisions/README.md)               | Architecture decision records               | You need the reasoning behind a choice. |

## Engine guides

| Document                                               | Contents                                      | Read it when              |
| ------------------------------------------------------ | --------------------------------------------- | ------------------------- |
| [Board representation](engine/board-representation.md) | Planned position model and bitboard rationale | Beginning Phase 1.        |
| [Move generation](engine/move-generation.md)           | Legal move correctness strategy               | Beginning Phase 2.        |
| [Search](engine/search.md)                             | Planned search progression                    | Working on search phases. |
| [Position evaluation](engine/position-evaluation.md)   | Classical evaluation direction                | Designing evaluation.     |
| [Transposition table](engine/transposition-table.md)   | Hashing and table design                      | Adding search caching.    |
| [Time management](engine/time-management.md)           | Planned clock-aware search                    | Integrating timed play.   |
| [UCI protocol](engine/uci-protocol.md)                 | Planned GUI protocol boundary                 | Preparing UCI support.    |

## Testing and performance

| Document                                | Contents                                    | Read it when                     |
| --------------------------------------- | ------------------------------------------- | -------------------------------- |
| [Testing](testing/testing.md)           | Test strategy and quality gates             | Adding or reviewing behavior.    |
| [Perft](testing/perft.md)               | Move-generation validation methodology      | Debugging legal moves.           |
| [Benchmarking](testing/benchmarking.md) | Reproducible measurement practices          | Comparing a change.              |
| [Performance](testing/performance.md)   | Performance design and profiling principles | Optimizing measured bottlenecks. |

## Engineering and references

| Document                                              | Contents                               | Read it when                       |
| ----------------------------------------------------- | -------------------------------------- | ---------------------------------- |
| [Code rules](engineering/code-rules.md)               | C++ and Python standards               | Writing project code or tooling.   |
| [Build](engineering/build.md)                         | Intended CMake build policy            | Introducing a build configuration. |
| [Naming and layout](engineering/naming-and-layout.md) | File naming and module-boundary policy | Adding project files or modules.   |
| [Glossary](reference/glossary.md)                     | Shared engine terminology              | A term is unfamiliar.              |
| [References](reference/references.md)                 | External primary and learning sources  | Researching a design.              |

## Future component boundaries

| Document                                                | Contents                                                         | Read it when                                 |
| ------------------------------------------------------- | ---------------------------------------------------------------- | -------------------------------------------- |
| [ML and research](ml/README.md)                         | Offline research boundary                                        | Planning experiments or model work.          |
| [Backend](backend/README.md)                            | Future service boundary                                          | Planning the application backend.            |
| [Frontend](frontend/README.md)                          | Future client boundary                                           | Planning the application frontend.           |
| [Operations](operations/README.md)                      | Future delivery and runtime boundary                             | Planning CI, deployment, or operations.      |
| [Legal](legal/README.md)                                | Legal-decision boundary                                          | Considering licensing or provenance work.    |
| [Licensing and notices](legal/LICENSING_AND_NOTICES.md) | Legal inventory, licence decision gate, and asset-review process | Reviewing licence or provenance status.      |
| [Third-party notices](legal/THIRD_PARTY_NOTICES.md)     | Current dependency and asset attribution inventory               | Adding or releasing a third-party component. |

Repository-level [contribution guidance](../CONTRIBUTING.md), [security policy](../SECURITY.md), and [agent instructions](../AGENTS.md) also apply.
