import React, { useState } from 'react';
import { PlayScreen } from './components/play/PlayScreen';
import { AnalysisScreen } from './components/analysis/AnalysisScreen';
import { GamesScreen } from './components/games/GamesScreen';
import { SettingsScreen } from './components/settings/SettingsScreen';
import type { SavedGame, UserSettings } from './api/types';

export type TabType = 'play' | 'analysis' | 'games' | 'settings';

export const App: React.FC = () => {
  const [activeTab, setActiveTab] = useState<TabType>('play');
  const [settings, setSettings] = useState<UserSettings>({
    boardTheme: 'standard',
    engineDepth: 8,
    engineMoveTimeMs: 1000,
    showLegalMoves: true,
    soundEnabled: false,
  });

  const [savedGames, setSavedGames] = useState<SavedGame[]>([
    {
      id: 'default-1',
      date: '2026-11-02',
      white: 'Morphy, Paul',
      black: 'Duke of Brunswick',
      result: '1-0',
      pgn: '1. e4 e5 2. Nf3 d6 3. d4 Bg4 4. dxe5 Bxf3 5. Qxf3 dxe5 6. Bc4 Nf6 7. Qb3 Qe7 8. Nc3 c6 9. Bg5 b5 10. Nxb5 cxb5 11. Bxb5+ Nbd7 12. O-O-O Rd8 13. Rxd7 Rxd7 14. Rd1 Qe6 15. Bxd7+ Nxd7 16. Qb8+ Nxb8 17. Rd8# 1-0',
      fen: '3Rkb1r/p4ppp/4q3/4p1B1/4P3/8/PPP2PPP/2K5 b k - 1 17',
    },
  ]);

  const handleSaveGame = (game: SavedGame) => {
    setSavedGames((prev) => [game, ...prev]);
  };

  const navItems: { id: TabType; label: string }[] = [
    { id: 'play', label: 'Play Engine' },
    { id: 'analysis', label: 'Analysis Board' },
    { id: 'games', label: 'Saved Games' },
    { id: 'settings', label: 'Settings' },
  ];

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex flex-col font-sans">
      {/* Navigation Bar */}
      <header className="border-b border-zinc-800 bg-zinc-900/60 sticky top-0 z-40 backdrop-blur-md px-6 py-3 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-gradient-to-tr from-blue-600 to-indigo-500 flex items-center justify-center font-bold text-white shadow">
            ♜
          </div>
          <div>
            <h1 className="text-base font-bold tracking-tight">RookSac</h1>
            <p className="text-[10px] text-zinc-400">Autonomous C++20 Chess Engine Platform</p>
          </div>
        </div>

        <nav className="flex space-x-1" role="tablist">
          {navItems.map((item) => (
            <button
              key={item.id}
              role="tab"
              aria-selected={activeTab === item.id}
              onClick={() => setActiveTab(item.id)}
              className={`px-3 py-1.5 rounded-md text-xs font-medium transition cursor-pointer ${
                activeTab === item.id
                  ? 'bg-zinc-800 text-white font-semibold shadow-xs'
                  : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800/50'
              }`}
            >
              {item.label}
            </button>
          ))}
        </nav>
      </header>

      {/* Main Screen View */}
      <main className="flex-1 overflow-y-auto">
        {activeTab === 'play' && (
          <PlayScreen settings={settings} onSaveGame={handleSaveGame} />
        )}
        {activeTab === 'analysis' && <AnalysisScreen />}
        {activeTab === 'games' && (
          <GamesScreen games={savedGames} onImportPgnSuccess={handleSaveGame} />
        )}
        {activeTab === 'settings' && (
          <SettingsScreen settings={settings} onUpdateSettings={setSettings} />
        )}
      </main>

      <footer className="border-t border-zinc-900 text-[11px] text-zinc-500 py-3 text-center">
        RookSac Frontend &bull; C++20 Native Engine via Asynchronous FastAPI Worker Pool
      </footer>
    </div>
  );
};

export default App;
