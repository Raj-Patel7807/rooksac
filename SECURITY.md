# Security policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | ✅ Yes    |

## Reporting a vulnerability

**Do not open a public GitHub issue for a security vulnerability.**

Report suspected vulnerabilities privately:

- **GitHub private security advisory** — use the "Report a vulnerability" button on the
  [Security tab](https://github.com/Raj-Patel7807/rooksac/security/advisories) of this
  repository (recommended).
- **Email** — rajpatel7807@gmail.com with subject line `[RookSac] Security report`.

Please include:

- A minimal reproduction or description of the issue.
- The affected version or commit hash.
- Estimated impact (memory safety, remote code execution, denial of service, etc.).
- Safe mitigation ideas if you have them.
- Do **not** include credentials, private keys, or sensitive personal data in reports.

## Scope

Potential concerns include:

- Memory-safety defects in the C++ engine (buffer overruns, use-after-free, undefined behavior).
- Unsafe parsing of FEN strings, UCI input, or PGN data.
- Dependency vulnerabilities introduced by `requirements.txt` or `package.json`.
- Build or release integrity (supply-chain risks).
- Future protocol-handling vulnerabilities.

## Response

This is a single-maintainer project with no formal response SLA or bug bounty programme.
Maintainers will triage reports promptly and aim to issue a fix or advisory within a reasonable
timeframe. Credit will be given in the release notes unless anonymity is requested.
