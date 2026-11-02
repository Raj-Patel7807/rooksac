import React, { useState } from 'react';
import { Chess } from 'chess.js';
import { Board } from '../board/Board';
import { MoveList } from './MoveList';
import { PromotionDialog } from '../board/PromotionDialog';
import { Button } from '../ui/Button';
import { apiClient } from '../../api/client';
import type { SavedGame, UserSettings } from '../../api/types';

export interface PlayScreenProps {
  settings: UserSettings;
  onSaveGame?: (game: SavedGame) => void;
}

export const PlayScreen: React.FC<PlayScreenProps> = ({ settings, onSaveGame }) => {
  const [game, setGame] = useState<Chess>(new Chess());
  const [fen, setFen] = useState<string>(game.fen());
  const [history, setHistory] = useState<string[]>([]);
  const [pendingMove, setPendingMove] = useState<{ from: string; to: string } | null>(null);
  const [isEngineThinking, setIsEngineThinking] = useState(false);
  const [gameStatus, setGameStatus] = useState<string>('White to move');
  const [showPromotion, setShowPromotion] = useState(false);

  // Compute status
  const updateStatus = (chessInstance: Chess) => {
    if (chessInstance.isCheckmate()) {
      setGameStatus(`Checkmate! ${chessInstance.turn() === 'w' ? 'Black' : 'White'} wins.`);
    } else if (chessInstance.isDraw()) {
      setGameStatus('Draw (stalemate or insufficient material).');
    } else if (chessInstance.inCheck()) {
      setGameStatus(`Check! ${chessInstance.turn() === 'w' ? 'White' : 'Black'} to move.`);
    } else {
      setGameStatus(`${chessInstance.turn() === 'w' ? 'White' : 'Black'} to move.`);
    }
  };

  const executeEngineMove = async (currentChess: Chess) => {
    if (currentChess.isGameOver()) return;
    setIsEngineThinking(true);
    try {
      const resp = await apiClient.requestAnalysis({
        fen: currentChess.fen(),
        depth: settings.engineDepth,
        movetime_ms: settings.engineMoveTimeMs,
      });

      if (resp.best_move && resp.best_move !== '(none)') {
        const from = resp.best_move.substring(0, 2);
        const to = resp.best_move.substring(2, 4);
        const promotion = resp.best_move.length > 4 ? resp.best_move[4] : undefined;

        const move = currentChess.move({ from, to, promotion });
        if (move) {
          setFen(currentChess.fen());
          setHistory(currentChess.history());
          updateStatus(currentChess);
        }
      }
    } catch (err) {
      console.error('Engine move error:', err);
    } finally {
      setIsEngineThinking(false);
    }
  };

  const handlePieceDrop = (sourceSquare: string, targetSquare: string, _piece: string): boolean => {
    if (isEngineThinking || game.isGameOver()) return false;

    // Check if it's a promotion
    const moves = game.moves({ verbose: true });
    const isPromo = moves.some(
      (m) => m.from === sourceSquare && m.to === targetSquare && m.promotion
    );

    if (isPromo) {
      setPendingMove({ from: sourceSquare, to: targetSquare });
      setShowPromotion(true);
      return false; // wait for modal selection
    }

    try {
      const move = game.move({
        from: sourceSquare,
        to: targetSquare,
      });

      if (move === null) return false;

      setFen(game.fen());
      setHistory(game.history());
      updateStatus(game);

      // Trigger engine reply if it's Black's turn and human is White
      if (!game.isGameOver() && game.turn() === 'b') {
        setTimeout(() => executeEngineMove(game), 250);
      }
      return true;
    } catch {
      return false;
    }
  };

  const handlePromotionSelect = (promotionPiece: 'q' | 'r' | 'b' | 'n') => {
    if (!pendingMove) return;
    try {
      const move = game.move({
        from: pendingMove.from,
        to: pendingMove.to,
        promotion: promotionPiece,
      });
      if (move) {
        setFen(game.fen());
        setHistory(game.history());
        updateStatus(game);
        if (!game.isGameOver() && game.turn() === 'b') {
          setTimeout(() => executeEngineMove(game), 250);
        }
      }
    } finally {
      setShowPromotion(false);
      setPendingMove(null);
    }
  };

  const handleRestart = () => {
    const newG = new Chess();
    setGame(newG);
    setFen(newG.fen());
    setHistory([]);
    setGameStatus('White to move');
  };

  const handleSaveGame = () => {
    if (!onSaveGame) return;
    const saved: SavedGame = {
      id: String(Date.now()),
      date: new Date().toISOString().split('T')[0],
      pgn: game.pgn() || '(Empty game)',
      white: 'Human Player',
      black: 'RookSac Engine',
      result: game.isGameOver()
        ? game.isCheckmate()
          ? game.turn() === 'w'
            ? '0-1'
            : '1-0'
          : '1/2-1/2'
        : '*',
      fen: game.fen(),
    };
    onSaveGame(saved);
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 items-start justify-center p-4 max-w-6xl mx-auto">
      <div className="flex flex-col items-center">
        <Board fen={fen} onPieceDrop={handlePieceDrop} />
        <div className="mt-3 text-sm font-medium text-zinc-300 bg-zinc-800 px-4 py-1.5 rounded-full border border-zinc-700">
          {isEngineThinking ? 'RookSac is thinking...' : gameStatus}
        </div>
      </div>

      <div className="w-full lg:w-80 flex flex-col gap-4">
        <MoveList moves={history} />

        <div className="flex flex-wrap gap-2">
          <Button variant="secondary" size="sm" onClick={handleRestart}>
            Restart
          </Button>
          <Button variant="outline" size="sm" onClick={handleSaveGame}>
            Save Game
          </Button>
        </div>
      </div>

      <PromotionDialog
        isOpen={showPromotion}
        color={game.turn()}
        onSelect={handlePromotionSelect}
        onCancel={() => setShowPromotion(false)}
      />
    </div>
  );
};
