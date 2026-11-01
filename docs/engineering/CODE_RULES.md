# Code rules

## C++20

- Use clear `PascalCase` types, `camelCase` functions and variables, and `kPascalCase` constants unless a local established convention supersedes this policy.
- Keep headers focused on declarations and put non-trivial definitions in source files. Minimize exposed dependencies.
- Place project code in the `rooksac` namespace; avoid global mutable state.
- Use `const` for values and member functions that do not mutate observable state.
- Model ownership explicitly. Prefer automatic storage, values, references, and RAII; use smart pointers only when ownership requires them.
- Avoid dynamic allocation and hidden work in performance-sensitive paths unless measurement justifies it.
- Validate external input at boundaries. Use return values or explicit result types for recoverable errors; use assertions for programmer invariants, not user input.
- Do not rely on undefined behavior, unchecked narrowing, uninitialized state, or lifetime assumptions.
- Comments should explain intent, invariants, or trade-offs—not restate syntax.
- Profile before optimizing. Preserve correctness and readable invariants before specializing hot code.

## Python tooling

- Target Python 3 and keep tools offline from the engine’s runtime search loop.
- Use type hints for non-trivial public functions, isolated environments, explicit inputs/outputs, and reproducible scripts where practical.
- Keep generated datasets, large artifacts, credentials, and environment-specific paths out of version control.
- Treat Python results as experiments: record data source, parameters, environment, and limitations.

See [performance principles](../testing/PERFORMANCE.md) and [build guidance](BUILD.md).
