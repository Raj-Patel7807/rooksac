import { describe, it, expect, vi, beforeEach } from "vitest";
import { RobustWebSocket } from "../api/websocket";

describe("RobustWebSocket", () => {
    let latestSocket: any;

    beforeEach(() => {
        vi.restoreAllMocks();

        class MockWebSocket {
            static OPEN = 1;
            static CONNECTING = 0;
            static CLOSING = 2;
            static CLOSED = 3;

            readyState = 1;
            send = vi.fn();
            close = vi.fn();
            onopen: any = null;
            onmessage: any = null;
            onclose: any = null;
            onerror: any = null;

            constructor() {
                latestSocket = this;
            }
        }

        (globalThis as any).WebSocket = MockWebSocket;
    });

    it("receives in-order messages and updates sequence", () => {
        const rws = new RobustWebSocket("ws://localhost:8000/ws");
        rws.connect();

        const received: any[] = [];
        rws.subscribe((msg) => received.push(msg));

        // Message 1
        latestSocket.onmessage?.({
            data: JSON.stringify({
                type: "engine_info",
                seq: 1,
                payload: { depth: 1 },
            }),
        });
        // Message 2
        latestSocket.onmessage?.({
            data: JSON.stringify({
                type: "engine_info",
                seq: 2,
                payload: { depth: 2 },
            }),
        });

        expect(received.length).toBe(2);
        expect(rws.getLastProcessedSeq()).toBe(2);
    });

    it("drops stale out-of-order messages", () => {
        const rws = new RobustWebSocket("ws://localhost:8000/ws");
        rws.connect();

        const received: any[] = [];
        rws.subscribe((msg) => received.push(msg));

        // Message seq 5
        latestSocket.onmessage?.({
            data: JSON.stringify({
                type: "engine_info",
                seq: 5,
                payload: { depth: 5 },
            }),
        });
        expect(rws.getLastProcessedSeq()).toBe(5);

        // Stale message seq 3 arrives later
        latestSocket.onmessage?.({
            data: JSON.stringify({
                type: "engine_info",
                seq: 3,
                payload: { depth: 3 },
            }),
        });

        // Should drop seq 3
        expect(received.length).toBe(1);
        expect(received[0].seq).toBe(5);
    });

    it("sends messages when open", () => {
        const rws = new RobustWebSocket("ws://localhost:8000/ws");
        rws.connect();
        rws.send({ type: "ping" });

        expect(latestSocket.send).toHaveBeenCalledWith(
            JSON.stringify({ type: "ping" }),
        );
    });
});
