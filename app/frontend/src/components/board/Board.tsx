import React from 'react';
import { Chessboard } from 'react-chessboard';
import type { PieceDropHandlerArgs } from 'react-chessboard';

export type BoardTheme = 'standard' | 'wood' | 'dark' | 'blue';

export interface BoardProps {
  fen: string;
  orientation?: 'white' | 'black';
  onPieceDrop?: (sourceSquare: string, targetSquare: string, piece: string) => boolean;
  onSquareClick?: (square: string) => void;
  customSquareStyles?: Record<string, React.CSSProperties>;
  boardWidth?: number;
  arePiecesDraggable?: boolean;
  theme?: BoardTheme;
}

const themeColors: Record<BoardTheme, { light: string; dark: string }> = {
  standard: { light: '#ebecd0', dark: '#739552' }, // Chess.com style green/buff
  wood: { light: '#e4c49f', dark: '#a97a44' },
  dark: { light: '#8c8c8c', dark: '#404040' },
  blue: { light: '#dee3e6', dark: '#8ca2ad' },
};

export const Board: React.FC<BoardProps> = ({
  fen,
  orientation = 'white',
  onPieceDrop,
  onSquareClick,
  customSquareStyles = {},
  boardWidth = 480,
  arePiecesDraggable = true,
  theme = 'standard',
}) => {
  const handlePieceDrop = ({ piece, sourceSquare, targetSquare }: PieceDropHandlerArgs): boolean => {
    if (!targetSquare || !onPieceDrop) return false;
    return onPieceDrop(sourceSquare, targetSquare, piece.pieceType);
  };

  const colors = themeColors[theme] || themeColors.standard;

  return (
    <div
      className="chess-board-wrapper shadow-2xl rounded-lg overflow-hidden flex justify-center items-center bg-zinc-900 border border-zinc-800 transition-all select-none"
      style={{ width: boardWidth, height: boardWidth, maxWidth: '100%' }}
      aria-label="Chessboard"
    >
      <Chessboard
        options={{
          position: fen,
          boardOrientation: orientation,
          allowDragging: arePiecesDraggable,
          onPieceDrop: handlePieceDrop,
          onSquareClick: onSquareClick ? ({ square }) => onSquareClick(square) : undefined,
          squareStyles: customSquareStyles,
          darkSquareStyle: { backgroundColor: colors.dark },
          lightSquareStyle: { backgroundColor: colors.light },
          animationDurationInMs: 200,
        }}
      />
    </div>
  );
};
