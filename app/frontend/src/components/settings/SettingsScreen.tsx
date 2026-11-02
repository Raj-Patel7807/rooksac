import React from 'react';
import type { UserSettings } from '../../api/types';
import { Card } from '../ui/Card';

export interface SettingsScreenProps {
  settings: UserSettings;
  onUpdateSettings: (newSettings: UserSettings) => void;
}

export const SettingsScreen: React.FC<SettingsScreenProps> = ({
  settings,
  onUpdateSettings,
}) => {
  const handleChange = <K extends keyof UserSettings>(key: K, value: UserSettings[K]) => {
    onUpdateSettings({
      ...settings,
      [key]: value,
    });
  };

  return (
    <div className="max-w-2xl mx-auto p-4 space-y-6">
      <Card title="Engine Preferences">
        <div className="space-y-4 text-sm">
          <div>
            <label className="block text-xs font-semibold text-zinc-400 mb-1">
              Search Depth (Ply): {settings.engineDepth}
            </label>
            <input
              type="range"
              min="1"
              max="20"
              value={settings.engineDepth}
              onChange={(e) => handleChange('engineDepth', Number(e.target.value))}
              className="w-full h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer"
            />
            <div className="flex justify-between text-[10px] text-zinc-500 mt-1">
              <span>Fast (1)</span>
              <span>Balanced (8)</span>
              <span>Deep (20)</span>
            </div>
          </div>

          <div>
            <label className="block text-xs font-semibold text-zinc-400 mb-1">
              Move Time Limit (ms): {settings.engineMoveTimeMs} ms
            </label>
            <input
              type="range"
              min="100"
              max="10000"
              step="100"
              value={settings.engineMoveTimeMs}
              onChange={(e) => handleChange('engineMoveTimeMs', Number(e.target.value))}
              className="w-full h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer"
            />
          </div>
        </div>
      </Card>

      <Card title="Display & Behavior">
        <div className="space-y-4 text-sm">
          <div className="flex items-center justify-between">
            <span className="text-zinc-300">Highlight Legal Moves</span>
            <input
              type="checkbox"
              checked={settings.showLegalMoves}
              onChange={(e) => handleChange('showLegalMoves', e.target.checked)}
              className="rounded bg-zinc-800 border-zinc-700 text-blue-600 focus:ring-blue-500 h-4 w-4"
            />
          </div>

          <div className="flex items-center justify-between">
            <span className="text-zinc-300">Audio Feedback</span>
            <input
              type="checkbox"
              checked={settings.soundEnabled}
              onChange={(e) => handleChange('soundEnabled', e.target.checked)}
              className="rounded bg-zinc-800 border-zinc-700 text-blue-600 focus:ring-blue-500 h-4 w-4"
            />
          </div>
        </div>
      </Card>
    </div>
  );
};
