import time
import uuid
from collections import defaultdict
from contextlib import asynccontextmanager
from typing import Optional
from fastapi import FastAPI, Request, Response, WebSocket, WebSocketDisconnect, status
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware

from app.backend.config import settings
from app.backend.routes import pool, router as api_router

# In-memory sliding window rate limiter
_request_counts = defaultdict(list)


def get_trusted_client_ip(request: Request) -> str:
    """
    Extracts client IP safely:
    Only trusts X-Forwarded-For if the direct peer connection originates from
    a configured TRUSTED_PROXIES IP. Otherwise uses request.client.host directly.
    """
    peer_ip = request.client.host if request.client else "unknown"
    if peer_ip in settings.TRUSTED_PROXIES:
        forwarded_for = request.headers.get("x-forwarded-for")
        if forwarded_for:
            # Client IP is the leftmost entry in X-Forwarded-For
            client_ip = forwarded_for.split(",")[0].strip()
            if client_ip:
                return client_ip
    return peer_ip


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Controlled startup
    yield
    # Clean shutdown of all persistent engine processes
    await pool.shutdown()


def create_app() -> FastAPI:
    is_prod = settings.ENVIRONMENT == "production"
    app = FastAPI(
        title=settings.PROJECT_NAME,
        version=settings.VERSION,
        docs_url=None if is_prod else "/docs",
        redoc_url=None if is_prod else "/redoc",
        openapi_url=None if is_prod else "/openapi.json",
        lifespan=lifespan,
    )

    # Security headers, request ID & rate limit middleware
    @app.middleware("http")
    async def add_security_headers_and_rate_limit(request: Request, call_next):
        request_id = request.headers.get("x-request-id") or str(uuid.uuid4())
        client_ip = get_trusted_client_ip(request)
        now = time.time()

        # Prune calls older than 60s
        window = _request_counts[client_ip]
        _request_counts[client_ip] = [t for t in window if now - t < 60.0]

        if len(_request_counts[client_ip]) >= settings.MAX_REQUESTS_PER_MINUTE:
            return JSONResponse(
                status_code=status.HTTP_429_TOO_MANY_REQUESTS,
                content={"detail": "Rate limit exceeded. Maximum requests per minute reached."},
                headers={"X-Request-ID": request_id},
            )

        _request_counts[client_ip].append(now)

        response: Response = await call_next(request)
        response.headers["X-Request-ID"] = request_id
        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["X-Frame-Options"] = "DENY"
        response.headers["X-XSS-Protection"] = "1; mode=block"
        response.headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains"
        response.headers["Referrer-Policy"] = "strict-origin-when-cross-origin"
        return response

    # Strict CORS configuration
    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.ALLOWED_ORIGINS,
        allow_credentials=True,
        allow_methods=["GET", "POST", "OPTIONS"],
        allow_headers=["Content-Type", "Authorization", "X-Request-ID"],
    )

    # WebSocket endpoint with origin verification and clean disconnect handling
    @app.websocket("/api/v1/ws/analysis")
    async def ws_analysis_endpoint(websocket: WebSocket):
        origin = websocket.headers.get("origin")
        if origin and origin not in settings.ALLOWED_ORIGINS:
            await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
            return

        await websocket.accept()
        try:
            while True:
                data = await websocket.receive_json()
                fen = data.get("fen", "")
                depth = data.get("depth", 8)
                movetime_ms = data.get("movetime_ms")

                result = await pool.execute_analysis(
                    fen=fen,
                    depth=depth,
                    movetime_ms=movetime_ms,
                    timeout=settings.JOB_TIMEOUT_SECONDS,
                )
                await websocket.send_json(result)
        except WebSocketDisconnect:
            pass
        except Exception:
            try:
                await websocket.close(code=status.WS_1011_INTERNAL_ERROR)
            except Exception:
                pass

    app.include_router(api_router, prefix=settings.API_PREFIX)
    return app


app = create_app()
