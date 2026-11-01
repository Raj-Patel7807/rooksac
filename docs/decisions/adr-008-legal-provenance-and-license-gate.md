# ADR-008 — Legal provenance and license decision gate

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

RookSac has no confirmed copyright holder, license selection, dependencies, source code, assets, datasets, or models. Publishing a license or notices without those facts would claim rights without evidence. Future engine, research, and application work can introduce materials with independent license and attribution obligations.

## Decision

Maintain a documented legal inventory and third-party notice record. Do not publish a `LICENSE`, `NOTICE`, copyright holder/year notice, or SPDX source header until the owner confirms the holder, year policy, license choice, and contributor policy. Record each future external component, asset, dataset, and model separately, and generate an SPDX or CycloneDX SBOM from resolved dependencies for release artifacts.

## Alternatives considered

- Select MIT or Apache-2.0 without owner confirmation.
- Add empty license or notice files as placeholders.
- Defer all legal documentation until code exists.

## Consequences

The repository remains unlicensed until an owner decision is recorded, which limits reuse but avoids unsupported legal claims. Contributors have a clear intake process before adding third-party material. Future releases gain a repeatable provenance and SBOM review step.
