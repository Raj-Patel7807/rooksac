import React, { useState } from 'react';
import type { SavedGame } from '../../api/types';
import { Button } from '../ui/Button';
import { apiClient } from '../../api/client';

export interface GamesScreenProps {
  games: SavedGame[];
  onImportPgnSuccess?: (game: SavedGame) => void;
}

export const GamesScreen: React.FC<GamesScreenProps> = ({ games, onImportPgnSuccess }) => {
  const [pgnText, setPgnText] = useState('');
  const [importStatus, setImportStatus] = useState<string | null>(null);

  const handleImport = async () => {
    if (!pgnText.trim()) return;
    try {
      setImportStatus('Importing PGN...');
      const resp = await apiClient.importPgn({ pgn_text: pgnText });
      const newGame: SavedGame = {
        id: resp.game_id,
        date: new Date().toISOString().split('T')[0],
        pgn: pgnText,
        white: 'Imported Player',
        black: 'Imported Player',
        result: resp.result,
        fen: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      };
      onImportPgnSuccess?.(newGame);
      setPgnText('');
      setImportStatus(`Successfully imported game (${resp.moves_count} moves).`);
    } catch (err: any) {
      setImportStatus(`Import failed: ${err.message || 'Malformed PGN'}`);
    }
  };

  return (
    <div className="flex flex-col gap-6 p-4 max-w-4xl mx-auto">
      {/* PGN Import Box */}
      <div className="bg-zinc-900 border border-zinc-800 rounded-lg p-4">
        <h3 className="text-sm font-semibold text-zinc-100 mb-2 uppercase tracking-wide">
          Import PGN
        </h3>
        <textarea
          rows={4}
          value={pgnText}
          onChange={(e) => setPgnText(e.target.value)}
          placeholder="Paste PGN game text here..."
          className="w-full bg-zinc-800 border border-zinc-700 rounded p-2.5 text-xs font-mono text-zinc-100 focus:outline-none focus:ring-1 focus:ring-blue-500 mb-3"
        />
        <div className="flex items-center justify-between">
          <Button variant="primary" size="sm" onClick={handleImport}>
            Import Game
          </Button>
          {importStatus && <span className="text-xs text-zinc-400">{importStatus}</span>}
        </div>
      </div>

      {/* Saved Games List */}
      <div className="bg-zinc-900 border border-zinc-800 rounded-lg p-4">
        <h3 className="text-sm font-semibold text-zinc-100 mb-3 uppercase tracking-wide">
          Saved Games Archive
        </h3>
        {games.length === 0 ? (
          <div className="text-zinc-500 text-xs italic py-6 text-center">
            No games saved yet. Play a game or import a PGN!
          </div>
        ) : (
          <div className="space-y-3">
            {games.map((g) => (
              <div
                key={g.id}
                className="bg-zinc-800/60 border border-zinc-700/60 p-3 rounded flex flex-col md:flex-row justify-between items-start md:items-center gap-2"
              >
                <div>
                  <div className="text-sm font-medium text-zinc-200">
                    {g.white} vs. {g.black}
                  </div>
                  <div className="text-xs text-zinc-400">
                    Date: {g.date} | Result: <span className="font-semibold">{g.result}</span>
                  </div>
                </div>
                <div className="flex gap-2">
                  <Button
                    variant="outline"
                    size="sm"
                    onClick={() => {
                      navigator.clipboard.writeText(g.pgn);
                      alert('PGN copied to clipboard');
                    }}
                  >
                    Copy PGN
                  </Button>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};
