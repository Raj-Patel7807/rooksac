from typing import List, Optional
from pydantic import BaseModel, Field

class AnalysisRequest(BaseModel):
    fen: str = Field(..., description="Position in Forsyth-Edwards Notation")
    depth: int = Field(default=8, ge=1, le=32, description="Target search depth")
    movetime_ms: Optional[int] = Field(default=None, ge=10, le=60000, description="Fixed move time in ms")

class AnalysisResponse(BaseModel):
    job_id: str
    status: str
    fen: str
    best_move: Optional[str] = None
    depth: Optional[int] = None
    nodes: Optional[int] = None
    nps: Optional[int] = None
    score_cp: Optional[int] = None

class HealthResponse(BaseModel):
    status: str
    version: str
    engine_ready: bool
    active_workers: int
    queue_size: int

class PgnImportRequest(BaseModel):
    pgn_text: str

class PgnImportResponse(BaseModel):
    game_id: str
    moves_count: int
    result: str
