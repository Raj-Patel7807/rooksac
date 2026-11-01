# Contributing to RookSac

Thank you for contributing. RookSac has a repository and documentation foundation, but no engine implementation; proposed implementation work should align with the [roadmap](docs/product/roadmap.md) and requirements register.

## Before changing code

Read the [architecture](docs/architecture/architecture.md), applicable engine guide, [code rules](docs/engineering/code-rules.md), [naming and layout](docs/engineering/naming-and-layout.md), and [agent instructions](AGENTS.md). Check existing code and open decisions rather than assuming design documents describe an implementation.

## Branches and commits

Use focused branches when the repository workflow adopts them. Keep commits small and coherent. Prefer imperative, scoped subjects such as `docs: clarify perft strategy` or `movegen: handle en passant legality`. Do not mix refactoring, behavior changes, and unrelated formatting without a reason.

## Pull requests

Describe the problem, approach, affected requirements, validation performed, and known limitations. Include benchmark conditions for a performance claim. Reviewers should be able to distinguish planned work from implemented work.

## Tests and documentation

Every behavior change needs suitable tests. Move-generation work needs perft coverage where applicable. Update documentation when interfaces, architecture, requirements, or workflows change; significant design changes require an ADR. Never report tests or benchmarks that were not run.

## Review expectations

Review favors correctness, maintainability, explicit ownership, portability, and evidence over cleverness. Keep changes within scope and resolve comments with facts or documented trade-offs.
