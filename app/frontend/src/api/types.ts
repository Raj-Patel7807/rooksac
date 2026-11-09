export interface AnalysisRequest {
    fen: string;
    depth?: number;
    movetime_ms?: number;
}

export interface AnalysisResponse {
    job_id: string;
    status: string;
    fen: string;
    best_move?: string | null;
    depth?: number | null;
    nodes?: number | null;
    nps?: number | null;
    score_cp?: number | null;
}

export interface HealthResponse {
    status: string;
    version: string;
    engine_ready: boolean;
    active_workers: number;
    queue_size: number;
}

export interface PgnImportRequest {
    pgn_text: string;
}

export interface PgnImportResponse {
    game_id: string;
    moves_count: number;
    result: string;
}

export interface SavedGame {
    id: string;
    date: string;
    pgn: string;
    white: string;
    black: string;
    result: string;
    fen: string;
}

export interface UserSettings {
    boardTheme: "standard" | "wood" | "dark";
    engineDepth: number;
    engineMoveTimeMs: number;
    showLegalMoves: boolean;
    soundEnabled: boolean;
}
