import type {
    AnalysisRequest,
    AnalysisResponse,
    HealthResponse,
    PgnImportRequest,
    PgnImportResponse,
} from "./types";

export class ApiError extends Error {
    status: number;
    constructor(status: number, message: string) {
        super(message);
        this.name = "ApiError";
        this.status = status;
    }
}

const API_BASE = "/api/v1";

async function handleResponse<T>(res: Response): Promise<T> {
    if (!res.ok) {
        let errorDetail = res.statusText;
        try {
            const data = await res.json();
            if (data && data.detail) {
                errorDetail =
                    typeof data.detail === "string"
                        ? data.detail
                        : JSON.stringify(data.detail);
            }
        } catch {
            // Non-JSON error payload
        }
        throw new ApiError(
            res.status,
            errorDetail || `Request failed with code ${res.status}`,
        );
    }
    return res.json() as Promise<T>;
}

export const apiClient = {
    async getHealth(signal?: AbortSignal): Promise<HealthResponse> {
        const res = await fetch(`${API_BASE}/health`, { signal });
        return handleResponse<HealthResponse>(res);
    },

    async requestAnalysis(
        req: AnalysisRequest,
        signal?: AbortSignal,
    ): Promise<AnalysisResponse> {
        const res = await fetch(`${API_BASE}/analysis`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(req),
            signal,
        });
        return handleResponse<AnalysisResponse>(res);
    },

    async importPgn(
        req: PgnImportRequest,
        signal?: AbortSignal,
    ): Promise<PgnImportResponse> {
        const res = await fetch(`${API_BASE}/pgn/import`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(req),
            signal,
        });
        return handleResponse<PgnImportResponse>(res);
    },
};
