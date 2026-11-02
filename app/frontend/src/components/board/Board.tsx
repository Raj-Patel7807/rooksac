import React from 'react';
import { Chessboard } from 'react-chessboard';
import type { PieceDropHandlerArgs } from 'react-chessboard';

export interface BoardProps {
  fen: string;
  orientation?: 'white' | 'black';
  onPieceDrop?: (sourceSquare: string, targetSquare: string, piece: string) => boolean;
  customSquareStyles?: Record<string, React.CSSProperties>;
  boardWidth?: number;
  arePiecesDraggable?: boolean;
}

export const Board: React.FC<BoardProps> = ({
  fen,
  orientation = 'white',
  onPieceDrop,
  customSquareStyles,
  boardWidth = 480,
  arePiecesDraggable = true,
}) => {
  const handlePieceDrop = ({ piece, sourceSquare, targetSquare }: PieceDropHandlerArgs): boolean => {
    if (!targetSquare || !onPieceDrop) return false;
    return onPieceDrop(sourceSquare, targetSquare, piece.pieceType);
  };

  return (
    <div
      className="chess-board-wrapper shadow-lg rounded overflow-hidden flex justify-center items-center bg-zinc-900 border border-zinc-700"
      style={{ width: boardWidth, height: boardWidth }}
      aria-label="Chessboard"
    >
      <Chessboard
        options={{
          position: fen,
          boardOrientation: orientation,
          allowDragging: arePiecesDraggable,
          onPieceDrop: handlePieceDrop,
          squareStyles: customSquareStyles,
          darkSquareStyle: { backgroundColor: '#779952' },
          lightSquareStyle: { backgroundColor: '#edeed1' },
        }}
      />
    </div>
  );
};
