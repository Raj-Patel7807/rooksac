import { describe, it, expect, vi, beforeEach } from "vitest";
import { apiClient } from "../api/client";

describe("apiClient", () => {
    beforeEach(() => {
        vi.restoreAllMocks();
    });

    it("fetches health status successfully", async () => {
        const mockHealth = {
            status: "healthy",
            version: "0.1.0",
            engine_ready: true,
            active_workers: 0,
            queue_size: 0,
        };

        vi.spyOn(globalThis, "fetch").mockResolvedValueOnce({
            ok: true,
            json: async () => mockHealth,
        } as Response);

        const res = await apiClient.getHealth();
        expect(res).toEqual(mockHealth);
        expect(globalThis.fetch).toHaveBeenCalledWith("/api/v1/health", {
            signal: undefined,
        });
    });

    it("handles analysis error responses properly", async () => {
        vi.spyOn(globalThis, "fetch").mockResolvedValueOnce({
            ok: false,
            status: 400,
            statusText: "Bad Request",
            json: async () => ({ detail: "Malformed FEN string provided" }),
        } as Response);

        await expect(
            apiClient.requestAnalysis({ fen: "invalid fen" }),
        ).rejects.toThrow("Malformed FEN string provided");
    });

    it("posts PGN import payload and parses response", async () => {
        const mockImport = {
            game_id: "test-uuid",
            moves_count: 12,
            result: "1-0",
        };

        vi.spyOn(globalThis, "fetch").mockResolvedValueOnce({
            ok: true,
            json: async () => mockImport,
        } as Response);

        const res = await apiClient.importPgn({ pgn_text: "1. e4 e5 1-0" });
        expect(res).toEqual(mockImport);
    });
});
