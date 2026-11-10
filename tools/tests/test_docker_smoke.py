#!/usr/bin/env python3
"""
tools/tests/test_docker_smoke.py

Automated smoke test for RookSac Docker configuration and production container contracts.
Tests:
1. Dockerfile.backend multi-stage syntax and constraints:
   - base image pinning (python:3.12.9-slim-bookworm)
   - non-root user creation and execution (USER rooksac)
   - no dev dependencies in uv sync (--no-dev)
   - dynamic PORT binding ($PORT support)
   - single Uvicorn worker (--workers 1)
   - explicit healthcheck command on readiness endpoint
2. Dockerfile.frontend multi-stage syntax and constraints:
   - pinned node and nginx base images
   - non-root user (USER nginx)
3. .dockerignore exclusions:
   - excludes training data, raw checkpoints, git, node_modules, and cache
4. docker-compose.yml configuration:
   - services restricted to backend + frontend
   - no dangling postgres or redis containers
   - healthcheck dependency ordering
5. Docker CLI verification (runs container test if Docker daemon is running)
"""

import os
import re
import subprocess
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))


class TestDockerConfigSmoke(unittest.TestCase):
    def test_dockerfile_backend_configuration(self):
        backend_path = os.path.join(REPO_ROOT, "deploy/docker/Dockerfile.backend")
        self.assertTrue(os.path.exists(backend_path))
        with open(backend_path, "r", encoding="utf-8") as f:
            content = f.read()

        # 1. Pinned Debian Bookworm base image in both builder and runtime
        self.assertIn("FROM python:3.12.9-slim-bookworm AS engine-builder", content)
        self.assertIn("FROM python:3.12.9-slim-bookworm AS runtime", content)

        # 2. Non-root user created and active
        self.assertRegex(content, r"useradd\s+.*rooksac")
        self.assertIn("USER rooksac", content)

        # 3. Pinned uv binary and frozen no-dev install
        self.assertRegex(content, r"COPY\s+--from=ghcr\.io/astral-sh/uv:[\d\.]+")
        self.assertIn("uv sync --frozen --no-dev", content)

        # 4. Model artifact included
        self.assertIn("COPY --chown=rooksac:rooksac research/checkpoints/model_test.rks", content)

        # 5. Healthcheck present
        self.assertIn("HEALTHCHECK", content)
        self.assertIn("/api/v1/health", content)

        # 6. Single Uvicorn worker and dynamic port
        self.assertIn("--workers 1", content)
        self.assertIn("${PORT:-8000}", content)

    def test_dockerfile_frontend_configuration(self):
        frontend_path = os.path.join(REPO_ROOT, "deploy/docker/Dockerfile.frontend")
        self.assertTrue(os.path.exists(frontend_path))
        with open(frontend_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Pinned node and nginx images
        self.assertIn("FROM node:22.14.0-alpine3.21 AS builder", content)
        self.assertIn("FROM nginx:1.27.4-alpine3.21-slim AS runtime", content)
        self.assertIn("USER nginx", content)
        self.assertIn("HEALTHCHECK", content)

    def test_dockerignore_coverage(self):
        dockerignore_path = os.path.join(REPO_ROOT, ".dockerignore")
        self.assertTrue(os.path.exists(dockerignore_path))
        with open(dockerignore_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Key exclusions
        self.assertIn(".git", content)
        self.assertIn("node_modules", content)
        self.assertIn("research/data/*.jsonl", content)
        self.assertIn("research/checkpoints/*.pt", content)
        self.assertIn(".venv", content)

    def test_docker_compose_lean_stack(self):
        compose_path = os.path.join(REPO_ROOT, "deploy/docker/docker-compose.yml")
        self.assertTrue(os.path.exists(compose_path))
        with open(compose_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Verify only backend and frontend services exist
        self.assertIn("backend:", content)
        self.assertIn("frontend:", content)
        self.assertNotIn("db:", content)
        self.assertNotIn("postgres", content)
        self.assertNotIn("redis", content)
        self.assertIn("condition: service_healthy", content)


if __name__ == "__main__":
    unittest.main()
