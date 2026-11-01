# Licensing and notices

This record describes the repository's current legal posture. It is not legal advice.

## Current status

No public license has been selected. The repository must not be treated as open source or redistributed on the assumption that a license will be added later. No copyright holder, copyright-year policy, or contributor-rights policy has been confirmed.

The inspected repository contains documentation and configuration only. It contains no C or C++ source files, copied implementation snippets, dependency manifests, lockfiles, generated files, datasets, model weights, fonts, icons, images, audio, archives, or binaries. Git history examined for this sprint contains the same documentation-only foundation and no evidence of a third-party implementation import. This is an inventory finding, not a claim that every future contribution is original.

The project references external learning resources, including Stockfish and Chess Programming Wiki pages. References are not distributed components and do not grant permission to copy code or other material. No ChessMinds source, asset, model, or other repository material is included.

## License decision gate

MIT and Apache-2.0 are both permissive options for the project's future original code.

| Option | Benefits | Trade-offs |
| --- | --- | --- |
| MIT | Short, widely understood, and allows broad reuse with preservation of the copyright and license notice. | It does not include an express patent grant. |
| Apache-2.0 | Permissive and includes an express patent license plus patent-termination terms. | It is longer and carries notice-handling obligations when distributed notices apply. |

For a simple permissive posture, MIT is the recommended option to evaluate. Apache-2.0 is the alternative to evaluate when an express patent grant is important. This is a recommendation only; neither license is selected.

Before adding a `LICENSE`, the owner must confirm all of the following:

- Exact copyright holder name and year policy.
- Whether any employer, university, competition, sponsor, or team agreement affects ownership or licensing.
- Chosen license: MIT, Apache-2.0, another identified license, or an intentional decision to remain unlicensed/private.
- Whether external contributions are accepted and, if so, the DCO, CLA, or equivalent contributor-rights policy.
- That included original work and every third-party component are compatible with the selected license.

After confirmation, add only the exact canonical license text, apply notices only to confirmed original material, and add SPDX identifiers to original source files only if the selected project policy requires them. Do not add a copyright notice or SPDX identifier to third-party material.

## Notice posture

No `NOTICE` file is present because no selected license or distributed component currently requires one. An empty notice file would falsely imply an obligation or attribution set. Reassess this decision when a selected license or third-party component requires notice preservation.

## Third-party, asset, dataset, and model review

Before adding any external component, create or update [third-party notices](THIRD_PARTY_NOTICES.md) with:

- Name, version, source URL or repository, and license identifier or terms URL.
- Category: runtime dependency, development dependency, optional tool, asset, dataset, or model artifact.
- Whether it is shipped, bundled, downloaded at runtime, or used only in development.
- Required attribution, license text, notice, source-offer, modification disclosure, or distribution restriction.
- Modifications made, if any, and compatibility with the selected project license.

Algorithms described in public material are ideas, not permission to copy implementations. Dataset and model rights are reviewed separately from source-code licensing. Record dataset terms, retrieval date, redistribution rights, checksum, and filtering; record model architecture, training-data manifest, training-code revision, checksum, and distribution terms.

## SBOM plan

When dependency manifests or release artifacts exist, generate an SPDX or CycloneDX SBOM from the resolved dependency set. Store the generated SBOM with the release artifact or release record, review it for missing licenses and unexpected components, and update the third-party notice inventory before release. The repository has no manifest or release artifact today, so no SBOM can yet be generated.

## Verification record

- File-system inventory found no source/header files, dependency manifests, lockfiles, binary assets, datasets, models, fonts, icons, images, audio, or archives.
- Git history and tracked content were searched for provenance and license terms; no copied implementation or ChessMinds material was found.
- No license text was added because the owner has not confirmed a license or copyright holder.
