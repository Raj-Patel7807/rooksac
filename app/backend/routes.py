import uuid
from typing import Dict, Optional
from fastapi import APIRouter, HTTPException, status

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
pool = BoundedEnginePool(settings.ENGINE_BINARY_PATH, max_workers=settings.MAX_CONCURRENT_WORKERS)

@router.get("/health", response_model=HealthResponse)
async def health_check():
    engine_ready = False
    try:
        worker = pool
        engine_ready = True
    except Exception:
        pass

    return HealthResponse(
        status="healthy",
        version=settings.VERSION,
        engine_ready=engine_ready,
        active_workers=pool.active_count,
        queue_size=0,
    )

@router.post("/analysis", response_model=AnalysisResponse)
async def request_analysis(req: AnalysisRequest):
    # Validate FEN
    if not req.fen or len(req.fen.split()) < 4:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Malformed FEN string provided"
        )

    job_id = str(uuid.uuid4())
    result = await pool.execute_analysis(
        fen=req.fen,
        depth=req.depth,
        movetime_ms=req.movetime_ms,
        timeout=settings.JOB_TIMEOUT_SECONDS,
    )

    if "error" in result:
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail=result["error"]
        )

    return AnalysisResponse(
        job_id=job_id,
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
            detail="PGN payload exceeds maximum permitted size"
        )

    parsed = parse_game(req.pgn_text)
    if not parsed:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Invalid or unparseable PGN game structure"
        )

    return PgnImportResponse(
        game_id=parsed["game_id"],
        moves_count=len(parsed["raw_text"].split()),
        result=str(parsed["score"]),
    )
