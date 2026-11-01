# Sprint 02 completion report

## A. Sprint summary

Sprint 02 establishes a reviewable legal and provenance baseline for RookSac. The Sprint-02 legal artifacts were already present as uncommitted working-tree changes when this execution began; they were validated and preserved. The repository is documented as unlicensed: neither a public `LICENSE` nor a `NOTICE`, copyright-holder notice, or SPDX source header has been added because the owner has not confirmed the copyright holder, year policy, contributor-rights policy, or licence choice.

The legal records inventory the current documentation-only repository, compare MIT and Apache-2.0, identify MIT as the permissive option to evaluate for a simple posture, and record Apache-2.0 as the alternative when an express patent grant matters. This is not a licence selection. The outcome is a repeatable intake process for future dependencies, assets, datasets, and models, plus an SBOM plan for release artifacts.

## B. Repository structure

Before Sprint 02, the tracked repository contained root governance/configuration documents and a documentation-only `docs/` tree. It had no engine sources, dependency manifests, lockfiles, generated files, datasets, model weights, fonts, icons, images, audio, archives, binaries, build configuration, tests, or CI workflows.

After Sprint 02, the documented structure adds the legal inventory and decision record:

```text
rooksac/
├── docs/
│   ├── decisions/
│   │   └── adr-008-legal-provenance-and-license-gate.md
│   └── legal/
│       ├── LICENSING_AND_NOTICES.md
│       └── THIRD_PARTY_NOTICES.md
├── README.md
└── tmp/Sprints/Sprint-02.md
```

No implementation directory or release artifact was added. The new legal documentation links the current repository facts to the future intake and release-review process.

## C. File-level changes

The following Sprint-02-scope files were present as uncommitted work when execution began. They were inspected and preserved; none was discarded or included in a commit by this execution.

- Created: `docs/legal/LICENSING_AND_NOTICES.md` — legal inventory, licence decision gate, review requirements for third-party materials, and SBOM plan.
- Created: `docs/legal/THIRD_PARTY_NOTICES.md` — explicit inventory showing that no distributed third-party components currently exist.
- Created: `docs/decisions/adr-008-legal-provenance-and-license-gate.md` — records the policy of deferring licence, copyright, notice, and SPDX publication until owner confirmation.
- Modified: `README.md` — links the licence status to the legal decision record.
- Modified: `docs/PROJECT_STATUS.md` — accurately marks the legal/provenance baseline as in progress and owner-dependent items as deferred.
- Modified: `docs/README.md` — indexes the two legal records.
- Modified: `docs/decisions/README.md` — indexes ADR-008.
- Modified: `docs/engineering/naming-and-layout.md` — documents the sprint-specified legal filenames as naming exceptions.
- Modified: `docs/legal/README.md` — links the legal inventory and states the unlicensed posture.
- Created: `tmp/Sprints/Sprint-02.md` — this completion report.

No files were moved, renamed, or deleted.

## D. Technical and architectural decisions

- The repository remains unlicensed until the owner confirms the exact copyright holder, year policy, licence choice, ownership constraints, and contributor-rights policy. This avoids asserting rights without evidence; the trade-off is that reuse and redistribution permissions remain unavailable.
- MIT is recommended only as the permissive option to evaluate when simplicity is the priority. Apache-2.0 remains the alternative when its express patent grant is important. Neither is selected, so no canonical licence text was verified or added.
- No `NOTICE` file was added. There is no selected licence or distributed component that requires one, and an empty notice could be misleading.
- No copyright notices or SPDX identifiers were added. No original source files exist, and the policy requires a confirmed licence before applying them.
- Future components must be inventoried by name, version, source, terms, distribution status, modifications, and notice obligations. Data and model rights are reviewed independently from code licences. A resolved dependency set will produce an SPDX or CycloneDX SBOM when release artifacts exist.
- Algorithms described publicly are treated as ideas, not permission to copy code. The inventory found no ChessMinds code, asset, model, or other repository material.

## E. Testing and validation

| Command | Status | Actual result |
| --- | --- | --- |
| `git status --short --branch` | Passed | On `main`; observed nine pre-existing Sprint-02-scope documentation changes, which were preserved. |
| `git log --oneline -8` and `git log --all --name-status --oneline -12` | Passed | Reviewed recent history; it contains only the Sprint-01 documentation foundation and no third-party implementation import. |
| `Get-ChildItem -Recurse -Force` and `git ls-files` | Passed | Reviewed the complete repository and tracked-file inventories. |
| `rg --files -g AGENTS.md -g README.md -g CONTRIBUTING.md -g '*.md' -g 'CMakeLists.txt' -g '*.yml' -g '*.yaml' -g '*.cpp' -g '*.hpp' -g '*.h' -g '*.py' -g '*.ps1' -g '.clang-format'` | Passed | Found repository guidance and documentation; no implementation, build, dependency, test, or workflow files. |
| `rg -n -i "chessminds|license|licen[cs]e|copyright|spdx|notice|third.?party|dependency|dataset|model|asset|font|icon|audio|generated|copy|provenance" . -g '!tmp/**' -g '!*.git/**'` | Passed | Reviewed provenance/licensing references and found no evidence of copied ChessMinds material. |
| PowerShell inventory of source/header files, manifests/lockfiles, and recognized binary/asset/model/archive extensions | Passed | 45 files outside `.git` and `tmp`; 0 source/header files, 0 dependency manifests/lockfiles, and 0 recognized binary/asset/model/archive files. |
| `rg -n --glob '!tmp/**' --glob '!.git/**' --pcre2 '(?i)(api[_-]?key|secret|password|token|private[_-]?key)\\s*[:=]\\s*[^\\s`"'"']+' .` | Passed | No secret-pattern matches. |
| `git diff --check` | Passed | No whitespace errors. |
| PowerShell local-Markdown-link checker across repository Markdown excluding `tmp/` | Passed | `Markdown local-link check passed for 41 files.` |
| `git diff --` plus `git diff --no-index -- /dev/null` for each new legal document | Passed | Reviewed every Sprint-02-scope documentation change. |
| Build, unit test, lint, formatter, static analysis, SBOM generation, and canonical licence-text verification | Not run / not applicable | The repository has no build system, source, tests, formatter target, dependency manifest, release artifact, selected SPDX licence, or confirmed copyright holder. |

## F. Git commits

- `1c9597c` — `docs: establish legal provenance baseline`: records the legal inventory, provenance decision gate, third-party notice inventory, and documentation cross-references.

This completion report is committed separately after the legal baseline so the two changes remain reviewable as focused units.

## G. Unresolved issues and risks

- Owner confirmation is still required for the copyright holder, year policy, ownership constraints, licence choice, and contributor-rights policy.
- No `LICENSE`, `NOTICE`, copyright notice, or SPDX source identifier can be added until those decisions are confirmed.
- The provenance finding is limited to the inspected repository and history; future contributions must follow the documented intake process.
- No SBOM can be generated until dependency manifests and release artifacts exist.

No implementation defects were identified because the repository contains no implementation.

## H. Sprint completion checklist

- [x] Inspected source files, copied snippets, generated files, datasets, model weights, assets, fonts, icons, sound files, dependencies, and licence headers; the current repository contains none of these material types.
- [x] `docs/legal/LICENSING_AND_NOTICES.md` is present with an inventory and owner decision checklist.
- [x] Explained MIT and Apache-2.0 trade-offs and recommended MIT only for evaluation; did not select or publish a licence.
- [x] Did not add a `LICENSE`, copyright notice, SPDX identifier, or `NOTICE` without owner confirmation.
- [x] `docs/legal/THIRD_PARTY_NOTICES.md` is present and distinguishes runtime, development/optional, generated-code, asset, dataset/opening-book, and model-artifact categories.
- [x] Documented a repeatable component/asset/data/model review process and an SPDX-or-CycloneDX SBOM generation plan.
- [x] Reviewed Git history and repository content for provenance; no ChessMinds material or other copied implementation was found.
- [x] Reported unresolved legal and provenance questions explicitly.
- [x] Performed all applicable repository validation; unavailable checks are explicitly identified above.

## I. Scope confirmation

- Work was performed directly on `main`.
- No branch was created, switched, or used other than `main`.
- No future sprint implementation was started.
- No unrelated user changes were committed, discarded, or overwritten. The pre-existing legal documentation was explicitly authorized for this sprint and committed as the legal-baseline unit above.
- All validation results in this report reflect commands actually executed.
- The initial instruction named `tmp/Sprints/Sprint-02.md` as the Sprint-02 report location. This report uses that location rather than the later conflicting reference to `Sprint-01.md`.
