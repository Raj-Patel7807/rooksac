# Naming and layout

## Naming

- Documentation and executable scripts use lowercase kebab-case names. Conventional repository index files remain `README.md`; `docs/PROJECT_STATUS.md`, `docs/legal/LICENSING_AND_NOTICES.md`, and `docs/legal/THIRD_PARTY_NOTICES.md` retain sprint-specified names.
- C++ source files use lowercase kebab-case when introduced; public headers belong under `engine/include/rooksac/` and implementation files under `engine/src/`.
- C++ follows the conventions in [code rules](code-rules.md): `PascalCase` types, `camelCase` functions and variables, `kPascalCase` constants, and namespace `rooksac`.

## Planned module boundaries

The repository is a modular monorepo. Directories are introduced only when they contain real work or a component-specific index.

| Area            | Purpose                                      | Dependency boundary                                                        |
| --------------- | -------------------------------------------- | -------------------------------------------------------------------------- |
| `engine/`       | Native C++ engine and its tests              | Builds and runs without Python, Node, databases, Redis, or network access. |
| `research/`     | Offline Python experiments and data analysis | May consume exported data; never enters the engine search path.            |
| `app/backend/`  | Future web/API orchestration                 | Invokes the engine through a UCI/process boundary.                         |
| `app/frontend/` | Future browser client                        | Talks only to documented application APIs.                                 |
| `benchmarks/`   | Reproducible benchmark inputs and harnesses  | Measures implemented behavior only.                                        |
| `tools/`        | Repository maintenance and developer tooling | Must not become an engine runtime dependency.                              |
| `deploy/`       | Future operational configuration             | Contains no secrets or generated environments.                             |

See [ADR 001](../decisions/ADR_001_ENGINE_ARCHITECTURE.md) for the decision and consequences.
