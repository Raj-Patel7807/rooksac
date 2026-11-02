import React, { useState } from 'react';
import { Chess } from 'chess.js';
import { Board } from '../board/Board';
import { EvalBar } from './EvalBar';
import { AnalysisPanel } from './AnalysisPanel';
import { Button } from '../ui/Button';
import { apiClient } from '../../api/client';
import type { AnalysisResponse } from '../../api/types';

export const AnalysisScreen: React.FC = () => {
  const [chess] = useState<Chess>(new Chess());
  const [fenInput, setFenInput] = useState<string>(chess.fen());
  const [boardFen, setBoardFen] = useState<string>(chess.fen());
  const [analysis, setAnalysis] = useState<AnalysisResponse | null>(null);
  const [loading, setLoading] = useState(false);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);
  const [abortController, setAbortController] = useState<AbortController | null>(null);

  const handleLoadFen = () => {
    try {
      chess.load(fenInput.trim());
      setBoardFen(chess.fen());
      setErrorMsg(null);
      setAnalysis(null);
    } catch {
      setErrorMsg('Invalid FEN format');
    }
  };

  const handleStartAnalysis = async () => {
    setLoading(true);
    setErrorMsg(null);
    const controller = new AbortController();
    setAbortController(controller);

    try {
      const resp = await apiClient.requestAnalysis(
        {
          fen: boardFen,
          depth: 10,
        },
        controller.signal
      );
      setAnalysis(resp);
    } catch (err: any) {
      if (err.name !== 'AbortError') {
        setErrorMsg(err.message || 'Analysis request failed');
      }
    } finally {
      setLoading(false);
      setAbortController(null);
    }
  };

  const handleStopAnalysis = () => {
    if (abortController) {
      abortController.abort();
      setAbortController(null);
    }
    setLoading(false);
  };

  const handlePieceDrop = (sourceSquare: string, targetSquare: string): boolean => {
    try {
      const move = chess.move({ from: sourceSquare, to: targetSquare, promotion: 'q' });
      if (move) {
        const nextFen = chess.fen();
        setBoardFen(nextFen);
        setFenInput(nextFen);
        setAnalysis(null);
        return true;
      }
    } catch {
      return false;
    }
    return false;
  };

  return (
    <div className="flex flex-col gap-6 p-4 max-w-5xl mx-auto items-center">
      <div className="w-full flex flex-col md:flex-row gap-2 items-center bg-zinc-900 border border-zinc-800 p-3 rounded-lg">
        <label htmlFor="fen-input" className="text-xs font-semibold text-zinc-400 whitespace-nowrap">
          FEN Position:
        </label>
        <input
          id="fen-input"
          type="text"
          value={fenInput}
          onChange={(e) => setFenInput(e.target.value)}
          className="flex-1 bg-zinc-800 border border-zinc-700 px-3 py-1.5 rounded text-xs font-mono text-zinc-100 focus:outline-none focus:ring-1 focus:ring-blue-500 w-full"
        />
        <Button variant="secondary" size="sm" onClick={handleLoadFen}>
          Load FEN
        </Button>
      </div>

      {errorMsg && (
        <div className="w-full text-xs text-red-400 bg-red-950/40 border border-red-800 p-2.5 rounded">
          {errorMsg}
        </div>
      )}

      <div className="flex flex-col md:flex-row items-center md:items-start gap-4">
        {/* Centipawn Evaluation Bar */}
        <div className="h-[480px]">
          <EvalBar scoreCp={analysis?.score_cp ?? 0} />
        </div>

        {/* Board */}
        <div>
          <Board fen={boardFen} onPieceDrop={handlePieceDrop} />
        </div>

        {/* Telemetry / Metrics Panel */}
        <div className="w-full md:w-64">
          <AnalysisPanel
            analysis={analysis}
            isLoading={loading}
            onAnalyze={handleStartAnalysis}
            onStop={handleStopAnalysis}
          />
        </div>
      </div>
    </div>
  );
};
