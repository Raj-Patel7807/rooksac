import { describe, it, expect, beforeEach, afterEach } from "vitest";
import {
    sanitizeUrl,
    httpToWsUrl,
    getApiBaseUrl,
    getWsBaseUrl,
} from "../config/env";

describe("env configuration helper", () => {
    const originalEnv = { ...import.meta.env };

    beforeEach(() => {
        // Reset env overrides
        delete (import.meta.env as any).VITE_API_BASE_URL;
        delete (import.meta.env as any).VITE_WS_BASE_URL;
    });

    afterEach(() => {
        Object.assign(import.meta.env, originalEnv);
    });

    it("sanitizes URLs correctly by removing trailing slashes", () => {
        expect(sanitizeUrl("https://example.com/api///")).toBe(
            "https://example.com/api",
        );
        expect(sanitizeUrl("http://localhost:8000/")).toBe(
            "http://localhost:8000",
        );
        expect(sanitizeUrl("/api/v1/")).toBe("/api/v1");
    });

    it("converts HTTP and HTTPS protocols to WS and WSS", () => {
        expect(httpToWsUrl("https://rooksac-api.onrender.com")).toBe(
            "wss://rooksac-api.onrender.com",
        );
        expect(httpToWsUrl("http://localhost:8000")).toBe(
            "ws://localhost:8000",
        );
        expect(httpToWsUrl("ws://already-ws.com")).toBe("ws://already-ws.com");
    });

    it("defaults getApiBaseUrl to relative /api/v1 when env var is not set", () => {
        expect(getApiBaseUrl()).toBe("/api/v1");
    });

    it("uses VITE_API_BASE_URL when configured", () => {
        (import.meta.env as any).VITE_API_BASE_URL =
            "https://rooksac-api.onrender.com/api/v1/";
        expect(getApiBaseUrl()).toBe("https://rooksac-api.onrender.com/api/v1");
    });

    it("derives WS URL from VITE_WS_BASE_URL when explicitly set", () => {
        (import.meta.env as any).VITE_WS_BASE_URL =
            "wss://rooksac-api.onrender.com/ws";
        expect(getWsBaseUrl()).toBe("wss://rooksac-api.onrender.com/ws");
    });

    it("derives WS URL from VITE_API_BASE_URL when VITE_WS_BASE_URL is absent", () => {
        (import.meta.env as any).VITE_API_BASE_URL =
            "https://rooksac-api.onrender.com";
        expect(getWsBaseUrl("/ws")).toBe(
            "wss://rooksac-api.onrender.com/ws",
        );
    });

    it("constructs correct browser fallback URL when window.location is defined", () => {
        const originalLocation = window.location;
        try {
            // @ts-ignore
            delete window.location;
            // @ts-ignore
            window.location = new URL("https://rooksac.vercel.app/play");
            expect(getWsBaseUrl("/ws")).toBe("wss://rooksac.vercel.app/ws");
        } finally {
            // @ts-ignore
            window.location = originalLocation;
        }
    });
});
