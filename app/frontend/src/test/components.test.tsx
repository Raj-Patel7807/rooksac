import { describe, it, expect } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { PromotionDialog } from '../components/board/PromotionDialog';
import { EvalBar } from '../components/analysis/EvalBar';
import { MoveList } from '../components/play/MoveList';

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
});
