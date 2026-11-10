import { getWsBaseUrl } from "../config/env";

export interface WsMessage {
    type: string;
    seq?: number;
    payload?: any;
}

export type WsHandler = (msg: WsMessage) => void;

export class RobustWebSocket {
    private url: string;
    private ws: WebSocket | null = null;
    private reconnectAttempts = 0;
    private maxReconnectAttempts = 5;
    private reconnectIntervalMs = 1000;
    private lastProcessedSeq = -1;
    private handlers: Set<WsHandler> = new Set();
    private isDisposed = false;

    constructor(url?: string) {
        this.url = url || getWsBaseUrl();
    }

    public connect(): void {
        if (this.isDisposed) return;

        try {
            this.ws = new WebSocket(this.url);

            this.ws.onopen = () => {
                this.reconnectAttempts = 0;
            };

            this.ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data) as WsMessage;
                    // Stale message / out-of-order drop
                    if (typeof data.seq === "number") {
                        if (data.seq <= this.lastProcessedSeq) {
                            // Discard stale message
                            return;
                        }
                        this.lastProcessedSeq = data.seq;
                    }
                    this.handlers.forEach((h) => h(data));
                } catch {
                    // ignore malformed payloads
                }
            };

            this.ws.onclose = () => {
                if (!this.isDisposed) {
                    this.scheduleReconnect();
                }
            };

            this.ws.onerror = () => {
                this.ws?.close();
            };
        } catch {
            this.scheduleReconnect();
        }
    }

    public send(msg: WsMessage): void {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(msg));
        }
    }

    public subscribe(handler: WsHandler): () => void {
        this.handlers.add(handler);
        return () => this.handlers.delete(handler);
    }

    public disconnect(): void {
        this.isDisposed = true;
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        this.handlers.clear();
    }

    private scheduleReconnect(): void {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            const delay =
                this.reconnectIntervalMs * Math.min(this.reconnectAttempts, 5);
            setTimeout(() => this.connect(), delay);
        }
    }

    public getLastProcessedSeq(): number {
        return this.lastProcessedSeq;
    }
}
