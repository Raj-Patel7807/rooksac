import React, { useState, useEffect, useRef, useCallback } from "react";
import { Chess } from "chess.js";
import type { Square } from "chess.js";
import { Board } from "../board/Board";
import type { BoardTheme } from "../board/Board";
import { MoveList } from "./MoveList";
import { PromotionDialog } from "../board/PromotionDialog";
import { PlayerHeader } from "./PlayerHeader";
import { GameOverModal } from "./GameOverModal";
import { TimeControlPicker, TIME_PRESETS } from "./TimeControlPicker";
import type { TimeControlPreset } from "./TimeControlPicker";
import { Button } from "../ui/Button";
import { apiClient } from "../../api/client";
import type {
    SavedGame,
    UserSettings,
    AnalysisResponse,
} from "../../api/types";
import { soundFx } from "../../utils/sound";
import {
    RotateCcw,
    Flag,
    Handshake,
    ArrowUpDown,
    Volume2,
    VolumeX,
    ChevronDown,
    ChevronUp,
    Cpu,
} from "lucide-react";

export interface PlayScreenProps {
    settings: UserSettings;
    onSaveGame?: (game: SavedGame) => void;
}

export const PlayScreen: React.FC<PlayScreenProps> = ({
    settings,
    onSaveGame,
}) => {
    const [game, setGame] = useState<Chess>(new Chess());
    const [fen, setFen] = useState<string>(game.fen());
    const [history, setHistory] = useState<string[]>([]);
    const [orientation, setOrientation] = useState<"white" | "black">("white");
    const [boardTheme, setBoardTheme] = useState<BoardTheme>("standard");
    const [soundOn, setSoundOn] = useState<boolean>(true);

    // Time controls
    const [timePreset, setTimePreset] = useState<TimeControlPreset>(
        TIME_PRESETS[3],
    ); // 3+2 default
    const [whiteTimeMs, setWhiteTimeMs] = useState<number>(
        timePreset.baseMinutes * 60 * 1000,
    );
    const [blackTimeMs, setBlackTimeMs] = useState<number>(
        timePreset.baseMinutes * 60 * 1000,
    );
    const [isClockRunning, setIsClockRunning] = useState<boolean>(false);

    // Interaction & UI state
    const [selectedSquare, setSelectedSquare] = useState<string | null>(null);
    const [legalTargetSquares, setLegalTargetSquares] = useState<string[]>([]);
    const [lastMove, setLastMove] = useState<{
        from: string;
        to: string;
    } | null>(null);
    const [pendingMove, setPendingMove] = useState<{
        from: string;
        to: string;
    } | null>(null);
    const [showPromotion, setShowPromotion] = useState(false);
    const [isEngineThinking, setIsEngineThinking] = useState(false);
    const [gameOverInfo, setGameOverInfo] = useState<{
        isOpen: boolean;
        winner: "white" | "black" | "draw" | null;
        reason: string;
    }>({ isOpen: false, winner: null, reason: "" });

    // Optional collapsible analysis info
    const [showAnalysis, setShowAnalysis] = useState<boolean>(false);
    const [latestEval, setLatestEval] = useState<AnalysisResponse | null>(null);

    // Captured pieces
    const [whiteCaptures, setWhiteCaptures] = useState<string[]>([]);
    const [blackCaptures, setBlackCaptures] = useState<string[]>([]);

    // Sync sound setting
    useEffect(() => {
        soundFx.setEnabled(soundOn);
    }, [soundOn]);

    // Compute captured pieces from current board
    const updateCaptures = useCallback((chess: Chess) => {
        const startCounts: Record<string, number> = {
            p: 8,
            n: 2,
            b: 2,
            r: 2,
            q: 1,
        };
        const currentCountsW: Record<string, number> = {
            p: 0,
            n: 0,
            b: 0,
            r: 0,
            q: 0,
        };
        const currentCountsB: Record<string, number> = {
            p: 0,
            n: 0,
            b: 0,
            r: 0,
            q: 0,
        };

        const board = chess.board();
        for (const row of board) {
            for (const piece of row) {
                if (piece && piece.type !== "k") {
                    if (piece.color === "w") {
                        currentCountsW[piece.type] =
                            (currentCountsW[piece.type] || 0) + 1;
                    } else {
                        currentCountsB[piece.type] =
                            (currentCountsB[piece.type] || 0) + 1;
                    }
                }
            }
        }

        const symbolMapW: Record<string, string> = {
            p: "♙",
            n: "♘",
            b: "♗",
            r: "♖",
            q: "♕",
        };
        const symbolMapB: Record<string, string> = {
            p: "♟",
            n: "♞",
            b: "♝",
            r: "♜",
            q: "♛",
        };

        const wCaps: string[] = [];
        const bCaps: string[] = [];

        for (const type of ["p", "n", "b", "r", "q"]) {
            const lostByBlack = Math.max(
                0,
                startCounts[type] - currentCountsB[type],
            );
            for (let i = 0; i < lostByBlack; ++i) wCaps.push(symbolMapB[type]);

            const lostByWhite = Math.max(
                0,
                startCounts[type] - currentCountsW[type],
            );
            for (let i = 0; i < lostByWhite; ++i) bCaps.push(symbolMapW[type]);
        }

        setWhiteCaptures(wCaps);
        setBlackCaptures(bCaps);
    }, []);

    // Clock countdown timer
    const lastTickRef = useRef<number>(Date.now());
    useEffect(() => {
        if (!isClockRunning || game.isGameOver()) return;

        lastTickRef.current = Date.now();
        const interval = setInterval(() => {
            const now = Date.now();
            const delta = now - lastTickRef.current;
            lastTickRef.current = now;

            if (game.turn() === "w") {
                setWhiteTimeMs((prev) => {
                    const next = Math.max(0, prev - delta);
                    if (next === 0) {
                        handleTimeOut("white");
                    }
                    return next;
                });
            } else {
                setBlackTimeMs((prev) => {
                    const next = Math.max(0, prev - delta);
                    if (next === 0) {
                        handleTimeOut("black");
                    }
                    return next;
                });
            }
        }, 100);

        return () => clearInterval(interval);
    }, [isClockRunning, game]);

    const handleTimeOut = (flaggedColor: "white" | "black") => {
        setIsClockRunning(false);
        soundFx.playGameOver();
        const winner = flaggedColor === "white" ? "black" : "white";
        setGameOverInfo({
            isOpen: true,
            winner,
            reason: `${flaggedColor === "white" ? "White" : "Black"} ran out of time`,
        });
    };

    const checkGameOverConditions = useCallback((chess: Chess) => {
        if (chess.isCheckmate()) {
            setIsClockRunning(false);
            soundFx.playGameOver();
            const winner = chess.turn() === "w" ? "black" : "white";
            setGameOverInfo({
                isOpen: true,
                winner,
                reason: "Checkmate",
            });
        } else if (chess.isDraw()) {
            setIsClockRunning(false);
            soundFx.playGameOver();
            let reason = "Draw";
            if (chess.isStalemate()) reason = "Stalemate";
            else if (chess.isThreefoldRepetition())
                reason = "Threefold repetition";
            else if (chess.isInsufficientMaterial())
                reason = "Insufficient material";
            setGameOverInfo({
                isOpen: true,
                winner: "draw",
                reason,
            });
        }
    }, []);

    // Trigger engine reply
    const executeEngineMove = async (currentChess: Chess) => {
        if (currentChess.isGameOver()) return;
        setIsEngineThinking(true);
        try {
            const resp = await apiClient.requestAnalysis({
                fen: currentChess.fen(),
                depth: settings.engineDepth,
                movetime_ms: settings.engineMoveTimeMs,
            });

            setLatestEval(resp);

            if (resp.best_move && resp.best_move !== "(none)") {
                const from = resp.best_move.substring(0, 2);
                const to = resp.best_move.substring(2, 4);
                const promotion =
                    resp.best_move.length > 4 ? resp.best_move[4] : undefined;

                const isCapture = currentChess.get(to as Square) !== null;
                const move = currentChess.move({ from, to, promotion });
                if (move) {
                    setFen(currentChess.fen());
                    setHistory(currentChess.history());
                    setLastMove({ from, to });
                    updateCaptures(currentChess);

                    // Add increment to Black's clock
                    setBlackTimeMs(
                        (t) => t + timePreset.incrementSeconds * 1000,
                    );

                    if (currentChess.inCheck()) {
                        soundFx.playCheck();
                    } else if (isCapture) {
                        soundFx.playCapture();
                    } else {
                        soundFx.playMove();
                    }

                    checkGameOverConditions(currentChess);
                }
            }
        } catch (err) {
            console.error("Engine move error:", err);
        } finally {
            setIsEngineThinking(false);
        }
    };

    // Perform a human move safely
    const makeMove = (
        from: string,
        to: string,
        promotion?: "q" | "r" | "b" | "n",
    ): boolean => {
        if (isEngineThinking || game.isGameOver()) return false;

        try {
            const isCapture = game.get(to as Square) !== null;
            const move = game.move({ from, to, promotion });
            if (!move) return false;

            // Start clock on first move
            if (!isClockRunning && history.length === 0) {
                setIsClockRunning(true);
            }

            // Add increment
            setWhiteTimeMs((t) => t + timePreset.incrementSeconds * 1000);

            setFen(game.fen());
            setHistory(game.history());
            setLastMove({ from, to });
            setSelectedSquare(null);
            setLegalTargetSquares([]);
            updateCaptures(game);

            if (game.inCheck()) {
                soundFx.playCheck();
            } else if (isCapture) {
                soundFx.playCapture();
            } else {
                soundFx.playMove();
            }

            checkGameOverConditions(game);

            if (!game.isGameOver() && game.turn() === "b") {
                setTimeout(() => executeEngineMove(game), 250);
            }
            return true;
        } catch {
            return false;
        }
    };

    // Piece drag and drop handler
    const handlePieceDrop = (
        sourceSquare: string,
        targetSquare: string,
    ): boolean => {
        if (isEngineThinking || game.isGameOver() || game.turn() !== "w")
            return false;

        // Check for promotion
        const moves = game.moves({ verbose: true });
        const isPromo = moves.some(
            (m) =>
                m.from === sourceSquare && m.to === targetSquare && m.promotion,
        );

        if (isPromo) {
            setPendingMove({ from: sourceSquare, to: targetSquare });
            setShowPromotion(true);
            return false;
        }

        return makeMove(sourceSquare, targetSquare);
    };

    // Square click handler for click-to-move
    const handleSquareClick = (square: string) => {
        if (isEngineThinking || game.isGameOver() || game.turn() !== "w")
            return;

        if (selectedSquare) {
            if (selectedSquare === square) {
                // Deselect
                setSelectedSquare(null);
                setLegalTargetSquares([]);
                return;
            }

            // Try move to square
            const moves = game.moves({ verbose: true });
            const isPromo = moves.some(
                (m) =>
                    m.from === selectedSquare && m.to === square && m.promotion,
            );

            if (isPromo) {
                setPendingMove({ from: selectedSquare, to: square });
                setShowPromotion(true);
                setSelectedSquare(null);
                setLegalTargetSquares([]);
                return;
            }

            const moved = makeMove(selectedSquare, square);
            if (moved) return;
        }

        // Select piece if it's White's
        const piece = game.get(square as Square);
        if (piece && piece.color === "w") {
            setSelectedSquare(square);
            const moves = game.moves({
                square: square as Square,
                verbose: true,
            });
            setLegalTargetSquares(moves.map((m) => m.to));
        } else {
            setSelectedSquare(null);
            setLegalTargetSquares([]);
        }
    };

    const handlePromotionSelect = (piece: "q" | "r" | "b" | "n") => {
        if (!pendingMove) return;
        makeMove(pendingMove.from, pendingMove.to, piece);
        setShowPromotion(false);
        setPendingMove(null);
    };

    // Start new game
    const handleNewGame = () => {
        const newG = new Chess();
        setGame(newG);
        setFen(newG.fen());
        setHistory([]);
        setSelectedSquare(null);
        setLegalTargetSquares([]);
        setLastMove(null);
        setIsClockRunning(false);
        setWhiteTimeMs(timePreset.baseMinutes * 60 * 1000);
        setBlackTimeMs(timePreset.baseMinutes * 60 * 1000);
        setWhiteCaptures([]);
        setBlackCaptures([]);
        setGameOverInfo({ isOpen: false, winner: null, reason: "" });
        setLatestEval(null);
    };

    // Resign
    const handleResign = () => {
        if (game.isGameOver()) return;
        setIsClockRunning(false);
        soundFx.playGameOver();
        setGameOverInfo({
            isOpen: true,
            winner: "black",
            reason: "White resigned",
        });
    };

    // Offer draw
    const handleOfferDraw = () => {
        if (game.isGameOver()) return;
        setIsClockRunning(false);
        soundFx.playGameOver();
        setGameOverInfo({
            isOpen: true,
            winner: "draw",
            reason: "Draw agreed",
        });
    };

    // Change time control
    const handleSelectTimeControl = (preset: TimeControlPreset) => {
        setTimePreset(preset);
        setWhiteTimeMs(preset.baseMinutes * 60 * 1000);
        setBlackTimeMs(preset.baseMinutes * 60 * 1000);
        handleNewGame();
    };

    // Save game
    const handleSaveGame = () => {
        if (!onSaveGame) return;
        const resultStr = gameOverInfo.winner
            ? gameOverInfo.winner === "white"
                ? "1-0"
                : gameOverInfo.winner === "black"
                  ? "0-1"
                  : "1/2-1/2"
            : "*";

        const saved: SavedGame = {
            id: String(Date.now()),
            date: new Date().toISOString().split("T")[0],
            pgn: game.pgn() || "(Empty game)",
            white: "Human Player",
            black: "RookSac Engine",
            result: resultStr,
            fen: game.fen(),
        };
        onSaveGame(saved);
    };

    // Custom square styles for highlights & legal moves
    const customSquareStyles: Record<string, React.CSSProperties> = {};

    if (lastMove) {
        customSquareStyles[lastMove.from] = {
            backgroundColor: "rgba(255, 255, 0, 0.35)",
        };
        customSquareStyles[lastMove.to] = {
            backgroundColor: "rgba(255, 255, 0, 0.45)",
        };
    }

    if (selectedSquare) {
        customSquareStyles[selectedSquare] = {
            backgroundColor: "rgba(125, 200, 255, 0.55)",
        };
    }

    if (settings.showLegalMoves) {
        for (const sq of legalTargetSquares) {
            const isOccupied = game.get(sq as Square) !== null;
            customSquareStyles[sq] = {
                background: isOccupied
                    ? "radial-gradient(circle, transparent 75%, rgba(0, 0, 0, 0.3) 80%)"
                    : "radial-gradient(circle, rgba(0, 0, 0, 0.25) 25%, transparent 30%)",
                borderRadius: "50%",
                cursor: "pointer",
            };
        }
    }

    return (
        <div className="flex flex-col lg:flex-row gap-6 items-start justify-center p-4 max-w-6xl mx-auto select-none">
            {/* Board & Player Area */}
            <div className="flex flex-col items-center w-full max-w-[500px]">
                {/* Opponent (Top) */}
                <PlayerHeader
                    name={
                        orientation === "white"
                            ? "RookSac Engine"
                            : "Human Player"
                    }
                    isEngine={orientation === "white"}
                    rating={orientation === "white" ? "2800" : "1500"}
                    timeMs={orientation === "white" ? blackTimeMs : whiteTimeMs}
                    isActive={
                        isClockRunning &&
                        (orientation === "white"
                            ? game.turn() === "b"
                            : game.turn() === "w")
                    }
                    capturedPieces={
                        orientation === "white" ? blackCaptures : whiteCaptures
                    }
                />

                {/* The Board */}
                <div className="my-1.5 w-full flex justify-center">
                    <Board
                        fen={fen}
                        orientation={orientation}
                        theme={boardTheme}
                        onPieceDrop={handlePieceDrop}
                        onSquareClick={handleSquareClick}
                        customSquareStyles={customSquareStyles}
                        boardWidth={480}
                        arePiecesDraggable={
                            !isEngineThinking && !game.isGameOver()
                        }
                    />
                </div>

                {/* Player (Bottom) */}
                <PlayerHeader
                    name={
                        orientation === "white"
                            ? "Human Player"
                            : "RookSac Engine"
                    }
                    isEngine={orientation !== "white"}
                    rating={orientation === "white" ? "1500" : "2800"}
                    timeMs={orientation === "white" ? whiteTimeMs : blackTimeMs}
                    isActive={
                        isClockRunning &&
                        (orientation === "white"
                            ? game.turn() === "w"
                            : game.turn() === "b")
                    }
                    capturedPieces={
                        orientation === "white" ? whiteCaptures : blackCaptures
                    }
                />

                {/* Quick in-game action bar */}
                <div className="flex items-center justify-between w-full mt-2 px-1 text-xs">
                    <div className="flex items-center gap-1.5">
                        <button
                            type="button"
                            onClick={() =>
                                setOrientation((o) =>
                                    o === "white" ? "black" : "white",
                                )
                            }
                            className="p-1.5 rounded hover:bg-zinc-800 text-zinc-400 hover:text-white transition flex items-center gap-1"
                            title="Flip Board"
                        >
                            <ArrowUpDown size={14} />
                            <span className="hidden sm:inline">Flip</span>
                        </button>
                        <button
                            type="button"
                            onClick={() => setSoundOn((s) => !s)}
                            className="p-1.5 rounded hover:bg-zinc-800 text-zinc-400 hover:text-white transition flex items-center gap-1"
                            title="Toggle Sound"
                        >
                            {soundOn ? (
                                <Volume2 size={14} />
                            ) : (
                                <VolumeX size={14} />
                            )}
                        </button>
                        <select
                            value={boardTheme}
                            onChange={(e) =>
                                setBoardTheme(e.target.value as BoardTheme)
                            }
                            className="bg-zinc-800 text-zinc-300 rounded px-2 py-1 text-xs border border-zinc-700 outline-none"
                        >
                            <option value="standard">Green Theme</option>
                            <option value="wood">Wood Theme</option>
                            <option value="blue">Blue Theme</option>
                            <option value="dark">Dark Theme</option>
                        </select>
                    </div>

                    <div className="flex items-center gap-1.5">
                        <Button
                            variant="outline"
                            size="sm"
                            onClick={handleOfferDraw}
                            disabled={game.isGameOver()}
                            className="text-xs py-1 px-2.5"
                        >
                            <Handshake size={14} className="mr-1" />
                            Draw
                        </Button>
                        <Button
                            variant="danger"
                            size="sm"
                            onClick={handleResign}
                            disabled={game.isGameOver()}
                            className="text-xs py-1 px-2.5"
                        >
                            <Flag size={14} className="mr-1" />
                            Resign
                        </Button>
                    </div>
                </div>
            </div>

            {/* Side Control & Move List Panel */}
            <div className="w-full lg:w-84 flex flex-col gap-4">
                {/* Move History */}
                <MoveList moves={history} />

                {/* Collapsible Engine Analysis Panel */}
                <div className="bg-zinc-900 border border-zinc-800 rounded-lg p-3">
                    <button
                        type="button"
                        onClick={() => setShowAnalysis((prev) => !prev)}
                        className="w-full flex items-center justify-between text-xs font-semibold text-zinc-400 uppercase tracking-wider hover:text-zinc-200"
                    >
                        <span className="flex items-center gap-1.5">
                            <Cpu size={14} />
                            Engine Analysis (Optional)
                        </span>
                        {showAnalysis ? (
                            <ChevronUp size={16} />
                        ) : (
                            <ChevronDown size={16} />
                        )}
                    </button>

                    {showAnalysis && (
                        <div className="mt-3 pt-3 border-t border-zinc-800 text-xs space-y-2 font-mono">
                            <div className="flex justify-between">
                                <span className="text-zinc-400">Status:</span>
                                <span className="text-zinc-200">
                                    {isEngineThinking
                                        ? "Thinking..."
                                        : game.turn() === "w"
                                          ? "White to move"
                                          : "Black to move"}
                                </span>
                            </div>
                            <div className="flex justify-between">
                                <span className="text-zinc-400">
                                    Evaluation:
                                </span>
                                <span className="text-zinc-200">
                                    {latestEval?.score_cp !== undefined &&
                                    latestEval?.score_cp !== null
                                        ? `${latestEval.score_cp > 0 ? "+" : ""}${(latestEval.score_cp / 100).toFixed(2)}`
                                        : "—"}
                                </span>
                            </div>
                            <div className="flex justify-between">
                                <span className="text-zinc-400">
                                    Depth / Nodes:
                                </span>
                                <span className="text-zinc-200">
                                    {latestEval?.depth
                                        ? `d${latestEval.depth} / ${latestEval.nodes?.toLocaleString()}n`
                                        : "—"}
                                </span>
                            </div>
                            <div className="flex justify-between">
                                <span className="text-zinc-400">
                                    Best Move:
                                </span>
                                <span className="text-green-400 font-bold">
                                    {latestEval?.best_move || "—"}
                                </span>
                            </div>
                        </div>
                    )}
                </div>

                {/* Time Control Picker */}
                <div className="bg-zinc-900 border border-zinc-800 rounded-lg p-3">
                    <TimeControlPicker
                        selectedId={timePreset.id}
                        onSelect={handleSelectTimeControl}
                        disabled={isClockRunning && history.length > 0}
                    />
                </div>

                {/* Game Actions */}
                <div className="flex flex-col gap-2">
                    <Button
                        variant="primary"
                        size="md"
                        onClick={handleNewGame}
                        className="w-full justify-center"
                    >
                        <RotateCcw size={16} className="mr-2" />
                        New Game
                    </Button>
                    <Button
                        variant="outline"
                        size="sm"
                        onClick={handleSaveGame}
                        className="w-full justify-center"
                    >
                        Save PGN
                    </Button>
                </div>
            </div>

            {/* Pawn Promotion Modal */}
            <PromotionDialog
                isOpen={showPromotion}
                color={game.turn()}
                onSelect={handlePromotionSelect}
                onCancel={() => {
                    setShowPromotion(false);
                    setPendingMove(null);
                }}
            />

            {/* Game Over Modal */}
            <GameOverModal
                isOpen={gameOverInfo.isOpen}
                winner={gameOverInfo.winner}
                reason={gameOverInfo.reason}
                onNewGame={handleNewGame}
                onReview={() =>
                    setGameOverInfo((prev) => ({ ...prev, isOpen: false }))
                }
            />
        </div>
    );
};
