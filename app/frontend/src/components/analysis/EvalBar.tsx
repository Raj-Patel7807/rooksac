import React from "react";

export interface EvalBarProps {
    scoreCp?: number | null; // centipawns from White's perspective
    orientation?: "white" | "black";
}

export const EvalBar: React.FC<EvalBarProps> = ({
    scoreCp = 0,
    orientation = "white",
}) => {
    const cp = scoreCp ?? 0;
    // Map centipawns (-1000 to +1000) into a percentage (0% to 100% white share)
    // Win probability sigmoid: 1 / (1 + 10^(-cp / 400))
    const winProb = 1 / (1 + Math.pow(10, -cp / 400));
    const whitePercent = Math.min(100, Math.max(0, Math.round(winProb * 100)));
    const blackPercent = 100 - whitePercent;

    const displayWhite = orientation === "white" ? whitePercent : blackPercent;
    const displayBlack = 100 - displayWhite;

    const formattedScore =
        cp > 0 ? `+${(cp / 100).toFixed(2)}` : `${(cp / 100).toFixed(2)}`;

    return (
        <div
            className="flex flex-col items-center w-8 h-full min-h-[300px] bg-zinc-800 rounded overflow-hidden border border-zinc-700 select-none relative"
            aria-label={`Evaluation: ${formattedScore}`}
            title={`Evaluation: ${formattedScore} cp`}
        >
            {/* Black's bar on top if white oriented */}
            <div
                className="w-full bg-zinc-950 transition-all duration-300"
                style={{
                    height: `${orientation === "white" ? displayBlack : displayWhite}%`,
                }}
            />
            {/* White's bar on bottom if white oriented */}
            <div
                className="w-full bg-zinc-200 transition-all duration-300"
                style={{
                    height: `${orientation === "white" ? displayWhite : displayBlack}%`,
                }}
            />
            <div className="absolute inset-x-0 bottom-2 text-center text-[10px] font-mono font-bold text-zinc-900 bg-white/70 rounded px-0.5 mx-1">
                {formattedScore}
            </div>
        </div>
    );
};
