# Third-party notices

This inventory records third-party material distributed with RookSac.

## Runtime dependencies (C++ engine)

RookSac's C++ engine has **no external runtime dependencies**. It uses only the C++20 standard
library.

## Runtime dependencies (Python backend)

Installed via `app/backend/requirements.txt`. These are **not bundled** — they are downloaded
by pip at build/deploy time. Refer to each package's own license for redistribution terms.

| Package | License | Notes |
|---------|---------|-------|
| fastapi | MIT | Web framework |
| uvicorn | BSD-3-Clause | ASGI server |
| pydantic | MIT | Data validation |
| pydantic-settings | MIT | Settings management |
| httpx | BSD-3-Clause | HTTP client (test client) |
| python-multipart | Apache-2.0 | Form data parsing |

## Runtime dependencies (Python ML research)

Installed separately (not in `requirements.txt`). Not shipped in the production Docker image.

| Package | License | Notes |
|---------|---------|-------|
| torch (PyTorch) | BSD-3-Clause | Neural network training |
| torchvision | BSD-3-Clause | Dataset utilities |
| numpy | BSD-3-Clause | Numerical arrays |

## Development and build tools

These are used during development only and are not distributed.

| Tool | License | Notes |
|------|---------|-------|
| CMake | BSD-3-Clause | Build system |
| Ninja | Apache-2.0 | Build driver |
| GCC / Clang | GPL-3.0 / Apache-2.0 | Compilers |
| Node.js / npm | MIT | Frontend tooling |
| Vite | MIT | Frontend bundler |
| Vitest | MIT | Frontend test runner |
| React | MIT | UI library |

## Frontend dependencies

Installed via `app/frontend/package.json`. Not bundled in source; downloaded at build time.
All production dependencies use MIT or BSD-compatible licenses. Run `npm licenses` in
`app/frontend/` for the full resolved list.

## Code snippets and generated code

No external code snippets are incorporated. Algorithms referenced from public computer-chess
literature (Chess Programming Wiki, public engine documentation) are used as study references
only — no code was copied from those sources.

## Assets, fonts, icons, and audio

None bundled.

## Datasets and opening books

None bundled.

## Model artifacts and weights

None bundled.

---

*Last reviewed: 2026-11-03. Update this file before adding any new component.*
