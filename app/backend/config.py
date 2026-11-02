import os
import shutil
from typing import List
from pydantic_settings import BaseSettings

def resolve_engine_binary() -> str:
    env_path = os.getenv("ENGINE_BINARY_PATH")
    if env_path and os.path.exists(env_path):
        return env_path
    
    # Check standard local and build locations
    candidates = [
        "build/rooksac",
        "build/rooksac.exe",
        "/opt/rooksac/bin/rooksac",
        "out/build/release/engine/apps/rooksac",
        "out/build/debug/engine/apps/rooksac",
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    
    # Fallback to system PATH or default
    which_path = shutil.which("rooksac")
    return which_path if which_path else candidates[0]

class Settings(BaseSettings):
    PROJECT_NAME: str = "RookSac Backend"
    VERSION: str = "1.0.0"
    API_PREFIX: str = "/api/v1"
    ENVIRONMENT: str = os.getenv("ENVIRONMENT", "production")
    
    # Engine orchestration
    ENGINE_BINARY_PATH: str = resolve_engine_binary()
    MAX_CONCURRENT_WORKERS: int = int(os.getenv("MAX_CONCURRENT_WORKERS", "4"))
    JOB_TIMEOUT_SECONDS: float = float(os.getenv("JOB_TIMEOUT_SECONDS", "30.0"))
    MAX_QUEUE_SIZE: int = int(os.getenv("MAX_QUEUE_SIZE", "100"))
    
    # Database and Cache (Private endpoints)
    DATABASE_URL: str = os.getenv("DATABASE_URL", "sqlite:///./rooksac.db")
    REDIS_URL: str = os.getenv("REDIS_URL", "redis://localhost:6379/0")
    
    # Quotas and Limits
    MAX_PGN_FILE_SIZE_BYTES: int = 2 * 1024 * 1024  # 2MB
    MAX_ANALYSIS_DEPTH: int = 32
    MAX_REQUESTS_PER_MINUTE: int = int(os.getenv("MAX_REQUESTS_PER_MINUTE", "120"))

    # CORS and Host Security
    ALLOWED_ORIGINS: List[str] = [
        origin.strip()
        for origin in os.getenv("ALLOWED_ORIGINS", "http://localhost:3000,http://127.0.0.1:3000").split(",")
        if origin.strip()
    ]

settings = Settings()
