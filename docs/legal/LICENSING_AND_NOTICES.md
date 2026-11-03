# Licensing and notices

This record describes the repository's legal posture. It is not legal advice.

## License

RookSac is released under the **MIT License**.

The canonical license text is in [`LICENSE`](../../LICENSE) at the repository root.
Copyright (c) 2026 Raj Patel.

SPDX identifier: `MIT`

## Copyright policy

- Copyright year: 2026 (the year the project was created).
- Copyright holder: Raj Patel.
- Future years are added to the `LICENSE` file as releases are made in those years.
- Third-party material retains its own copyright; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Contributor rights

Contributions submitted for inclusion in RookSac are accepted under the project's MIT License.
By opening a pull request, contributors certify that they have the right to submit the work
and agree that it may be distributed under the MIT License terms.

A formal CLA is not required at this time. The [Developer Certificate of Origin (DCO)](https://developercertificate.org/)
applies implicitly: contributors are responsible for ensuring their submissions do not violate
third-party rights.

## NOTICE file

A [`NOTICE`](../../NOTICE) file is included at the repository root. The MIT License does not
require a NOTICE file, but it is provided as a courtesy to record project attribution and to
serve as a placeholder for future third-party notice obligations.

## Third-party, asset, dataset, and model review

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the current inventory.

Before adding any external component, update that file with:

- Name, version, source URL, and SPDX license identifier.
- Category: runtime dependency, development dependency, optional tool, asset, dataset, or model.
- Whether it is shipped, bundled, downloaded at runtime, or development-only.
- Required attribution, license text, notice, source-offer, modification disclosure, or restriction.
- Compatibility with MIT.

Algorithms described in public chess-programming literature are ideas, not permission to copy
implementations. Dataset and model rights are reviewed separately from source-code licensing.

## SBOM

When dependency manifests or release artifacts exist, generate an SPDX or CycloneDX SBOM from
the resolved dependency set and store it with the release record. No formal SBOM has been
generated yet because no resolved lockfile is committed.

## Verification record

- MIT License added to `LICENSE` at repository root — 2026-11-03.
- `NOTICE` added at repository root — 2026-11-03.
- `AUTHORS` added at repository root — 2026-11-03.
- File-system inventory: no copied third-party source code, assets, datasets, or model weights
  bundled at time of initial release.
- Git history searched for provenance; no copied external implementation found.
