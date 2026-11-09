import React from "react";

export interface MoveListProps {
    moves: string[];
    currentMoveIndex?: number;
    onSelectMove?: (index: number) => void;
}

export const MoveList: React.FC<MoveListProps> = ({
    moves,
    currentMoveIndex = -1,
    onSelectMove,
}) => {
    // Group into pairs (White, Black)
    const pairs: Array<{ num: number; white: string; black?: string }> = [];
    for (let i = 0; i < moves.length; i += 2) {
        pairs.push({
            num: Math.floor(i / 2) + 1,
            white: moves[i],
            black: moves[i + 1],
        });
    }

    return (
        <div
            className="bg-zinc-900 border border-zinc-800 rounded-lg p-3 h-64 overflow-y-auto font-mono text-sm"
            aria-label="Move history"
        >
            <div className="text-xs font-semibold text-zinc-400 mb-2 uppercase tracking-wider">
                Move History
            </div>
            {pairs.length === 0 ? (
                <div className="text-zinc-500 italic text-center py-6">
                    No moves played yet
                </div>
            ) : (
                <div className="grid grid-cols-[30px_1fr_1fr] gap-y-1">
                    {pairs.map((pair, pIdx) => {
                        const whiteIdx = pIdx * 2;
                        const blackIdx = pIdx * 2 + 1;
                        return (
                            <React.Fragment key={pair.num}>
                                <span className="text-zinc-500">
                                    {pair.num}.
                                </span>
                                <button
                                    type="button"
                                    onClick={() => onSelectMove?.(whiteIdx)}
                                    className={`text-left px-1.5 py-0.5 rounded cursor-pointer ${
                                        currentMoveIndex === whiteIdx
                                            ? "bg-blue-600 text-white font-bold"
                                            : "text-zinc-200 hover:bg-zinc-800"
                                    }`}
                                >
                                    {pair.white}
                                </button>
                                {pair.black ? (
                                    <button
                                        type="button"
                                        onClick={() => onSelectMove?.(blackIdx)}
                                        className={`text-left px-1.5 py-0.5 rounded cursor-pointer ${
                                            currentMoveIndex === blackIdx
                                                ? "bg-blue-600 text-white font-bold"
                                                : "text-zinc-200 hover:bg-zinc-800"
                                        }`}
                                    >
                                        {pair.black}
                                    </button>
                                ) : (
                                    <span />
                                )}
                            </React.Fragment>
                        );
                    })}
                </div>
            )}
        </div>
    );
};
