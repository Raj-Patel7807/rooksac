import uuid
from typing import Dict, Optional
import chess
from fastapi import APIRouter, Header, HTTPException, Request, status

from app.backend.config import settings
from app.backend.engine_manager import BoundedEnginePool
from app.backend.schemas import (
    AnalysisRequest,
    AnalysisResponse,
    HealthResponse,
    PgnImportRequest,
    PgnImportResponse,
)
from tools.pgn_extract import parse_game

router = APIRouter()
pool = BoundedEnginePool(
    binary_path=settings.ENGINE_BINARY_PATH,
    max_workers=settings.MAX_CONCURRENT_WORKERS,
    max_queue_size=settings.MAX_QUEUE_SIZE,
    eval_mode=settings.ENGINE_EVAL_MODE,
    model_path=settings.ENGINE_MODEL_PATH,
    use_policy=settings.ENGINE_USE_POLICY,
    neural_weight=settings.ENGINE_NEURAL_WEIGHT,
    hash_mb=settings.ENGINE_HASH_MB,
)


@router.get("/health", response_model=HealthResponse)
async def health_check():
    """
    Readiness endpoint: probes active engine worker to confirm real process responsiveness
    and reports model status and classical fallback.
    """
    engine_ready, model_ready, fallback_active, model_details = await pool.ping_readiness()

    return HealthResponse(
        status="healthy" if engine_ready else "degraded",
        version=settings.VERSION,
        engine_ready=engine_ready,
        model_ready=model_ready,
        eval_mode=settings.ENGINE_EVAL_MODE,
        fallback_active=fallback_active,
        model_details=model_details if model_details else None,
        active_workers=pool.active_count,
        queue_size=pool.waiting_count,
        max_workers=pool.max_workers,
    )


@router.post("/analysis", response_model=AnalysisResponse)
async def request_analysis(
    req: AnalysisRequest,
    request: Request,
    x_request_id: Optional[str] = Header(None),
):
    request_id = x_request_id or str(uuid.uuid4())

    # 1. Rigorous FEN validation with python-chess (rejects malformed strings and shell escapes)
    try:
        chess.Board(req.fen)
    except Exception:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Malformed FEN string provided",
        )

    # 2. Bound depth and movetime limits
    if req.depth < 1 or req.depth > settings.MAX_ANALYSIS_DEPTH:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Requested depth {req.depth} exceeds limits [1, {settings.MAX_ANALYSIS_DEPTH}]",
        )

    # 3. Execute analysis through bounded pool
    result = await pool.execute_analysis(
        fen=req.fen,
        depth=req.depth,
        movetime_ms=req.movetime_ms,
        timeout=settings.JOB_TIMEOUT_SECONDS,
    )

    if result.get("queue_full"):
        raise HTTPException(
            status_code=status.HTTP_429_TOO_MANY_REQUESTS,
            detail="Engine queue full. Please retry shortly.",
        )

    if result.get("timeout"):
        raise HTTPException(
            status_code=status.HTTP_504_GATEWAY_TIMEOUT,
            detail="Analysis request timed out.",
        )

    if "error" in result:
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail="Engine analysis service unavailable.",
        )

    return AnalysisResponse(
        job_id=request_id,
        status="completed",
        fen=req.fen,
        best_move=result.get("best_move"),
        depth=result.get("depth"),
        nodes=result.get("nodes"),
        nps=result.get("nps"),
        score_cp=result.get("score_cp"),
    )


@router.post("/pgn/import", response_model=PgnImportResponse)
async def import_pgn(req: PgnImportRequest):
    if len(req.pgn_text.encode("utf-8")) > settings.MAX_PGN_FILE_SIZE_BYTES:
        raise HTTPException(
            status_code=status.HTTP_413_REQUEST_ENTITY_TOO_LARGE,
            detail="PGN payload exceeds maximum permitted size",
        )

    parsed = parse_game(req.pgn_text)
    if not parsed:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Invalid or unparseable PGN game structure",
        )

    return PgnImportResponse(
        game_id=parsed["game_id"],
        moves_count=len(parsed["raw_text"].split()),
        result=str(parsed["score"]),
    )
