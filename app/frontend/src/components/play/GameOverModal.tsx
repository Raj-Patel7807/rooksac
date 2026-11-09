import React from 'react';
import { Trophy, Award, RotateCcw } from 'lucide-react';
import { Button } from '../ui/Button';

export interface GameOverModalProps {
  isOpen: boolean;
  winner: 'white' | 'black' | 'draw' | null;
  reason: string;
  onNewGame: () => void;
  onReview: () => void;
}

export const GameOverModal: React.FC<GameOverModalProps> = ({
  isOpen,
  winner,
  reason,
  onNewGame,
  onReview,
}) => {
  if (!isOpen) return null;

  const title =
    winner === 'draw'
      ? 'Game Drawn'
      : `${winner === 'white' ? 'White' : 'Black'} Won!`;

  return (
    <div
      role="dialog"
      aria-modal="true"
      aria-label="Game Over"
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/75 backdrop-blur-xs p-4 animate-in fade-in duration-200"
    >
      <div className="bg-zinc-900 border border-zinc-700 rounded-xl p-6 max-w-sm w-full shadow-2xl text-center flex flex-col items-center">
        <div
          className={`w-14 h-14 rounded-full flex items-center justify-center mb-3 ${
            winner === 'draw'
              ? 'bg-zinc-800 text-zinc-300'
              : 'bg-amber-500/20 text-amber-400 border border-amber-500/30'
          }`}
        >
          {winner === 'draw' ? <Award size={32} /> : <Trophy size={32} />}
        </div>

        <h2 className="text-xl font-bold text-white mb-1">{title}</h2>
        <p className="text-xs text-zinc-400 mb-6 capitalize">{reason}</p>

        <div className="flex flex-col gap-2.5 w-full">
          <Button variant="primary" size="md" onClick={onNewGame} className="w-full justify-center">
            <RotateCcw size={16} className="mr-1.5" />
            Play Again
          </Button>
          <Button variant="outline" size="md" onClick={onReview} className="w-full justify-center">
            Review Game
          </Button>
        </div>
      </div>
    </div>
  );
};
