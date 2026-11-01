# Instructions for coding agents

## Source of truth

1. Read the root [README](README.md) and relevant material in [docs](docs/README.md) before changing anything.
2. Inspect the current implementation, tests, build configuration, and Git diff before proposing modifications.
3. The implementation, tests, and actual configuration define current behavior. Documentation must describe reality, not replace it.

## Scope and architecture

- Work only within the requested scope; do not implement unrelated features.
- Follow the documented architecture and do not silently change it.
- Record significant architectural changes in [DECISIONS.md](docs/architecture/DECISIONS.md).
- Do not invent functionality or state that a planned feature exists.
- Keep C++ search and real-time move selection independent of Python.

## Engineering quality

- Prefer simple, correct, maintainable C++20.
- Add proportionate tests for every feature and run the tests you claim passed.
- Do not claim validation that was not run.
- Update relevant documentation whenever behavior or design changes.
- Measure before optimizing; retain before/after evidence for performance changes.
- Avoid unnecessary dependencies, abstraction, allocation, and generated artifacts.
- Never commit secrets, credentials, local settings, or generated build output.

## Workflow

1. Confirm the relevant requirement and roadmap phase.
2. Make the smallest coherent change.
3. Validate it at the appropriate level.
4. Inspect the final diff for scope, correctness, and documentation impact.
5. Report current behavior, validation actually performed, and remaining limitations plainly.
