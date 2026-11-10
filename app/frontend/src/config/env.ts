/**
 * Environment configuration for RookSac Frontend.
 *
 * Provides typed access to backend API and WebSocket endpoints:
 * - In local development (Vite dev server), relative URLs (/api/v1, /ws) route
 *   through Vite's proxy to the local backend.
 * - In production (e.g. Vercel deployment), VITE_API_BASE_URL and VITE_WS_BASE_URL
 *   allow targeting remote backends (e.g. Render).
 * - If unset in production, relative paths fall back to same-origin proxies (e.g. Docker/Nginx).
 *
 * NOTE: Never expose secrets in client-side VITE_* environment variables!
 */

export interface EnvConfig {
    apiBaseUrl: string;
    wsBaseUrl: string;
    isDev: boolean;
    isProd: boolean;
}

/**
 * Trims trailing slashes from a URL string.
 */
export function sanitizeUrl(url: string): string {
    return url.replace(/\/+$/, "");
}

/**
 * Converts an HTTP/HTTPS URL into its corresponding WS/WSS URL.
 */
export function httpToWsUrl(httpUrl: string): string {
    if (httpUrl.startsWith("https://")) {
        return "wss://" + httpUrl.slice("https://".length);
    }
    if (httpUrl.startsWith("http://")) {
        return "ws://" + httpUrl.slice("http://".length);
    }
    return httpUrl;
}

/**
 * Resolves the API Base URL for HTTP requests.
 * Defaults to "/api/v1" if VITE_API_BASE_URL is not set or in development mode.
 */
export function getApiBaseUrl(): string {
    const customApi = import.meta.env?.VITE_API_BASE_URL;
    if (customApi && typeof customApi === "string" && customApi.trim().length > 0) {
        return sanitizeUrl(customApi.trim());
    }
    return "/api/v1";
}

/**
 * Resolves the WebSocket URL for live telemetry/analysis.
 * - If VITE_WS_BASE_URL is set, uses it directly (e.g. wss://rooksac-api.onrender.com/ws).
 * - Otherwise, if VITE_API_BASE_URL is set, derives the WS endpoint from it.
 * - Otherwise, in browser environment, derives from window.location (ws:// or wss://).
 * - Falls back to relative/localhost ws endpoint.
 */
export function getWsBaseUrl(path: string = "/ws"): string {
    const customWs = import.meta.env?.VITE_WS_BASE_URL;
    if (customWs && typeof customWs === "string" && customWs.trim().length > 0) {
        const base = sanitizeUrl(customWs.trim());
        const cleanPath = path.startsWith("/") ? path : `/${path}`;
        // If customWs already contains the path suffix, don't duplicate
        if (base.endsWith(cleanPath)) {
            return base;
        }
        return `${base}${cleanPath}`;
    }

    const customApi = import.meta.env?.VITE_API_BASE_URL;
    if (customApi && typeof customApi === "string" && customApi.trim().length > 0) {
        const wsOrigin = httpToWsUrl(sanitizeUrl(customApi.trim()));
        const cleanPath = path.startsWith("/") ? path : `/${path}`;
        return `${wsOrigin}${cleanPath}`;
    }

    // Browser environment fallback (same host reverse proxy e.g. Docker/Nginx or Vite proxy)
    if (typeof window !== "undefined" && window.location) {
        const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
        const host = window.location.host;
        const cleanPath = path.startsWith("/") ? path : `/${path}`;
        return `${protocol}//${host}${cleanPath}`;
    }

    return `ws://localhost:8000${path.startsWith("/") ? path : `/${path}`}`;
}

export const envConfig: EnvConfig = {
    get apiBaseUrl() {
        return getApiBaseUrl();
    },
    get wsBaseUrl() {
        return getWsBaseUrl();
    },
    isDev: Boolean(import.meta.env?.DEV),
    isProd: Boolean(import.meta.env?.PROD),
};
