import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { PromotionDialog } from '../components/board/PromotionDialog';
import { EvalBar } from '../components/analysis/EvalBar';
import { MoveList } from '../components/play/MoveList';
import { PlayerHeader } from '../components/play/PlayerHeader';
import { GameOverModal } from '../components/play/GameOverModal';
import { TimeControlPicker, TIME_PRESETS } from '../components/play/TimeControlPicker';

describe('UI Components', () => {
  it('renders PromotionDialog and handles selection', () => {
    let selected: string | null = null;
    let cancelled = false;

    render(
      <PromotionDialog
        isOpen={true}
        color="w"
        onSelect={(p) => {
          selected = p;
        }}
        onCancel={() => {
          cancelled = true;
        }}
      />
    );

    expect(screen.getByRole('dialog')).toBeInTheDocument();
    const queenBtn = screen.getByLabelText('Promote to Queen');
    expect(queenBtn).toBeInTheDocument();
    fireEvent.click(queenBtn);
    expect(selected).toBe('q');

    const cancelBtn = screen.getByText('Cancel');
    fireEvent.click(cancelBtn);
    expect(cancelled).toBe(true);
  });

  it('renders EvalBar with formatted score', () => {
    render(<EvalBar scoreCp={150} />);
    expect(screen.getByLabelText('Evaluation: +1.50')).toBeInTheDocument();
  });

  it('renders MoveList properly', () => {
    const moves = ['e4', 'e5', 'Nf3', 'Nc6'];
    render(<MoveList moves={moves} />);

    expect(screen.getByText('1.')).toBeInTheDocument();
    expect(screen.getByText('e4')).toBeInTheDocument();
    expect(screen.getByText('e5')).toBeInTheDocument();
    expect(screen.getByText('2.')).toBeInTheDocument();
    expect(screen.getByText('Nf3')).toBeInTheDocument();
    expect(screen.getByText('Nc6')).toBeInTheDocument();
  });

  it('renders PlayerHeader with clock and handles active pulse', () => {
    render(
      <PlayerHeader
        name="Carlsen, Magnus"
        rating={2850}
        timeMs={180000}
        isActive={true}
        capturedPieces={['♟', '♟']}
      />
    );

    expect(screen.getByText('Carlsen, Magnus')).toBeInTheDocument();
    expect(screen.getByText('(2850)')).toBeInTheDocument();
    expect(screen.getByLabelText('Carlsen, Magnus Clock: 3:00')).toBeInTheDocument();
    expect(screen.getByText('♟♟')).toBeInTheDocument();
  });

  it('renders GameOverModal and triggers callbacks', () => {
    const handleNewGame = vi.fn();
    const handleReview = vi.fn();

    render(
      <GameOverModal
        isOpen={true}
        winner="white"
        reason="Checkmate"
        onNewGame={handleNewGame}
        onReview={handleReview}
      />
    );

    expect(screen.getByText('White Won!')).toBeInTheDocument();
    expect(screen.getByText('Checkmate')).toBeInTheDocument();

    fireEvent.click(screen.getByText('Play Again'));
    expect(handleNewGame).toHaveBeenCalledTimes(1);

    fireEvent.click(screen.getByText('Review Game'));
    expect(handleReview).toHaveBeenCalledTimes(1);
  });

  it('renders TimeControlPicker and triggers preset selection', () => {
    const handleSelect = vi.fn();

    render(
      <TimeControlPicker
        selectedId="3+2"
        onSelect={handleSelect}
      />
    );

    expect(screen.getByText('3 | 2')).toBeInTheDocument();
    const rapid10 = screen.getByText('10 min');
    fireEvent.click(rapid10);

    expect(handleSelect).toHaveBeenCalledWith(
      expect.objectContaining({ id: '10+0', baseMinutes: 10, incrementSeconds: 0 })
    );
  });
});
