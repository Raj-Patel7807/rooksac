import React from "react";
import type { AnalysisResponse } from "../../api/types";
import { Button } from "../ui/Button";

export interface AnalysisPanelProps {
    analysis: AnalysisResponse | null;
    isLoading: boolean;
    onAnalyze: () => void;
    onStop: () => void;
}

export const AnalysisPanel: React.FC<AnalysisPanelProps> = ({
    analysis,
    isLoading,
    onAnalyze,
    onStop,
}) => {
    return (
        <div className="bg-zinc-900 border border-zinc-800 rounded-lg p-4 font-mono text-sm">
            <div className="flex items-center justify-between mb-3 border-b border-zinc-800 pb-2">
                <h3 className="font-semibold text-zinc-100 uppercase tracking-wide text-xs">
                    Engine Analysis
                </h3>
                {isLoading ? (
                    <Button variant="danger" size="sm" onClick={onStop}>
                        Stop
                    </Button>
                ) : (
                    <Button variant="primary" size="sm" onClick={onAnalyze}>
                        Analyze
                    </Button>
                )}
            </div>

            {isLoading ? (
                <div className="text-zinc-400 text-xs py-4 flex items-center justify-center space-x-2">
                    <span className="inline-block w-2 h-2 rounded-full bg-blue-500 animate-ping" />
                    <span>Searching best move...</span>
                </div>
            ) : analysis ? (
                <div className="space-y-2 text-xs">
                    <div className="flex justify-between border-b border-zinc-800/50 py-1">
                        <span className="text-zinc-400">Best Move:</span>
                        <span className="font-bold text-green-400">
                            {analysis.best_move || "None"}
                        </span>
                    </div>
                    <div className="flex justify-between border-b border-zinc-800/50 py-1">
                        <span className="text-zinc-400">Score:</span>
                        <span className="text-zinc-200">
                            {analysis.score_cp !== undefined &&
                            analysis.score_cp !== null
                                ? `${analysis.score_cp > 0 ? "+" : ""}${(analysis.score_cp / 100).toFixed(2)}`
                                : "N/A"}
                        </span>
                    </div>
                    <div className="flex justify-between border-b border-zinc-800/50 py-1">
                        <span className="text-zinc-400">Depth:</span>
                        <span className="text-zinc-200">
                            {analysis.depth ?? "N/A"}
                        </span>
                    </div>
                    <div className="flex justify-between border-b border-zinc-800/50 py-1">
                        <span className="text-zinc-400">Nodes:</span>
                        <span className="text-zinc-200">
                            {analysis.nodes?.toLocaleString() ?? "N/A"}
                        </span>
                    </div>
                    <div className="flex justify-between py-1">
                        <span className="text-zinc-400">NPS:</span>
                        <span className="text-zinc-200">
                            {analysis.nps?.toLocaleString() ?? "N/A"}
                        </span>
                    </div>
                </div>
            ) : (
                <div className="text-zinc-500 text-xs italic py-4 text-center">
                    Click "Analyze" to inspect position
                </div>
            )}
        </div>
    );
};
