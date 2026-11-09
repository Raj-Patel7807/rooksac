import React from "react";

export interface PromotionDialogProps {
    isOpen: boolean;
    color: "w" | "b";
    onSelect: (piece: "q" | "r" | "b" | "n") => void;
    onCancel: () => void;
}

export const PromotionDialog: React.FC<PromotionDialogProps> = ({
    isOpen,
    color,
    onSelect,
    onCancel,
}) => {
    if (!isOpen) return null;

    const pieces = [
        { type: "q", label: "Queen", symbol: color === "w" ? "♕" : "♛" },
        { type: "r", label: "Rook", symbol: color === "w" ? "♖" : "♜" },
        { type: "b", label: "Bishop", symbol: color === "w" ? "♗" : "♝" },
        { type: "n", label: "Knight", symbol: color === "w" ? "♘" : "♞" },
    ] as const;

    return (
        <div
            role="dialog"
            aria-modal="true"
            aria-label="Pawn Promotion"
            className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-xs"
        >
            <div className="bg-zinc-800 border border-zinc-700 rounded-lg p-5 max-w-xs w-full shadow-xl text-center">
                <h3 className="text-lg font-semibold text-white mb-4">
                    Promote Pawn
                </h3>
                <div className="flex justify-around gap-2 mb-4">
                    {pieces.map((p) => (
                        <button
                            key={p.type}
                            onClick={() => onSelect(p.type)}
                            className="text-4xl p-3 bg-zinc-700 hover:bg-zinc-600 rounded transition border border-zinc-600 hover:border-blue-500 focus:outline-none focus:ring-2 focus:ring-blue-400"
                            aria-label={`Promote to ${p.label}`}
                        >
                            {p.symbol}
                        </button>
                    ))}
                </div>
                <button
                    onClick={onCancel}
                    className="text-xs text-zinc-400 hover:text-zinc-200 underline mt-2"
                >
                    Cancel
                </button>
            </div>
        </div>
    );
};
