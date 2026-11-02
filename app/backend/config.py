import os
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    PROJECT_NAME: str = "RookSac Backend"
    VERSION: str = "0.1.0"
    API_PREFIX: str = "/api/v1"
    
    # Engine orchestration
    ENGINE_BINARY_PATH: str = os.getenv("ENGINE_BINARY_PATH", "build/rooksac.exe")
    MAX_CONCURRENT_WORKERS: int = int(os.getenv("MAX_CONCURRENT_WORKERS", "4"))
    JOB_TIMEOUT_SECONDS: float = float(os.getenv("JOB_TIMEOUT_SECONDS", "30.0"))
    MAX_QUEUE_SIZE: int = int(os.getenv("MAX_QUEUE_SIZE", "100"))
    
    # Database and Cache
    DATABASE_URL: str = os.getenv("DATABASE_URL", "sqlite:///./rooksac.db")
    REDIS_URL: str = os.getenv("REDIS_URL", "redis://localhost:6379/0")
    
    # Quotas and Rate Limits
    MAX_PGN_FILE_SIZE_BYTES: int = 2 * 1024 * 1024 # 2MB
    MAX_ANALYSIS_DEPTH: int = 32

settings = Settings()
