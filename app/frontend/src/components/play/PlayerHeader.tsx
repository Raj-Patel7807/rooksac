import React from 'react';
import { User, Cpu } from 'lucide-react';

export interface PlayerHeaderProps {
  name: string;
  isEngine?: boolean;
  avatarIcon?: 'human' | 'engine';
  rating?: number | string;
  timeMs: number;
  isActive: boolean;
  capturedPieces?: string[];
  advantage?: number; // material diff
}

function formatClock(timeMs: number): string {
  if (timeMs < 0) timeMs = 0;
  const totalSeconds = Math.floor(timeMs / 1000);
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  const tenths = Math.floor((timeMs % 1000) / 100);

  if (minutes < 1 && totalSeconds < 20) {
    // Show tenths under 20 seconds
    return `${seconds}.${tenths}`;
  }
  return `${minutes}:${seconds.toString().padStart(2, '0')}`;
}

export const PlayerHeader: React.FC<PlayerHeaderProps> = ({
  name,
  isEngine = false,
  rating = 1500,
  timeMs,
  isActive,
  capturedPieces = [],
  advantage = 0,
}) => {
  const isLowTime = timeMs < 10000 && timeMs > 0;

  return (
    <div
      className={`w-full flex items-center justify-between px-3 py-2 rounded-md transition-all ${
        isActive ? 'bg-zinc-800/80 border border-zinc-700' : 'bg-transparent border border-transparent'
      }`}
    >
      {/* Player info */}
      <div className="flex items-center gap-2.5 min-w-0">
        <div
          className={`w-8 h-8 rounded flex items-center justify-center font-bold text-sm shrink-0 ${
            isEngine
              ? 'bg-amber-600/30 text-amber-400 border border-amber-500/30'
              : 'bg-emerald-600/30 text-emerald-400 border border-emerald-500/30'
          }`}
        >
          {isEngine ? <Cpu size={18} /> : <User size={18} />}
        </div>
        <div className="truncate">
          <div className="flex items-center gap-1.5">
            <span className="font-semibold text-sm text-zinc-100 truncate">{name}</span>
            <span className="text-[11px] text-zinc-400 font-mono">({rating})</span>
          </div>
          {capturedPieces.length > 0 && (
            <div className="flex items-center gap-0.5 text-xs text-zinc-400 leading-none">
              <span>{capturedPieces.join('')}</span>
              {advantage > 0 && <span className="text-[10px] text-zinc-300 font-bold ml-1">+{advantage}</span>}
            </div>
          )}
        </div>
      </div>

      {/* Clock */}
      <div
        className={`font-mono text-base font-bold px-3 py-1 rounded transition-colors tabular-nums ${
          isActive
            ? isLowTime
              ? 'bg-red-950 text-red-400 border border-red-700 animate-pulse'
              : 'bg-zinc-100 text-zinc-950 shadow-sm'
            : 'bg-zinc-900 text-zinc-400 border border-zinc-800'
        }`}
        aria-label={`${name} Clock: ${formatClock(timeMs)}`}
      >
        {formatClock(timeMs)}
      </div>
    </div>
  );
};
