import React from "react";
import { Clock } from "lucide-react";

export interface TimeControlPreset {
    id: string;
    label: string;
    category: "Bullet" | "Blitz" | "Rapid" | "Classical";
    baseMinutes: number;
    incrementSeconds: number;
}

export const TIME_PRESETS: TimeControlPreset[] = [
    {
        id: "1+0",
        label: "1 min",
        category: "Bullet",
        baseMinutes: 1,
        incrementSeconds: 0,
    },
    {
        id: "1+1",
        label: "1 | 1",
        category: "Bullet",
        baseMinutes: 1,
        incrementSeconds: 1,
    },
    {
        id: "3+0",
        label: "3 min",
        category: "Blitz",
        baseMinutes: 3,
        incrementSeconds: 0,
    },
    {
        id: "3+2",
        label: "3 | 2",
        category: "Blitz",
        baseMinutes: 3,
        incrementSeconds: 2,
    },
    {
        id: "5+0",
        label: "5 min",
        category: "Blitz",
        baseMinutes: 5,
        incrementSeconds: 0,
    },
    {
        id: "10+0",
        label: "10 min",
        category: "Rapid",
        baseMinutes: 10,
        incrementSeconds: 0,
    },
    {
        id: "10+5",
        label: "10 | 5",
        category: "Rapid",
        baseMinutes: 10,
        incrementSeconds: 5,
    },
    {
        id: "15+10",
        label: "15 | 10",
        category: "Rapid",
        baseMinutes: 15,
        incrementSeconds: 10,
    },
    {
        id: "30+0",
        label: "30 min",
        category: "Classical",
        baseMinutes: 30,
        incrementSeconds: 0,
    },
];

export interface TimeControlPickerProps {
    selectedId: string;
    onSelect: (preset: TimeControlPreset) => void;
    disabled?: boolean;
}

export const TimeControlPicker: React.FC<TimeControlPickerProps> = ({
    selectedId,
    onSelect,
    disabled = false,
}) => {
    return (
        <div className="flex flex-col gap-2">
            <div className="flex items-center gap-1.5 text-xs font-semibold text-zinc-400 uppercase tracking-wider">
                <Clock size={13} />
                <span>Time Control</span>
            </div>
            <div className="grid grid-cols-3 gap-1.5">
                {TIME_PRESETS.map((tc) => {
                    const isSelected = tc.id === selectedId;
                    return (
                        <button
                            key={tc.id}
                            type="button"
                            disabled={disabled}
                            onClick={() => onSelect(tc)}
                            className={`px-2 py-1.5 rounded text-xs font-medium flex flex-col items-center transition cursor-pointer disabled:opacity-50 disabled:cursor-not-allowed ${
                                isSelected
                                    ? "bg-blue-600 text-white font-bold shadow-xs"
                                    : "bg-zinc-800 text-zinc-300 hover:bg-zinc-700 hover:text-white"
                            }`}
                        >
                            <span>{tc.label}</span>
                            <span
                                className={`text-[9px] ${isSelected ? "text-blue-200" : "text-zinc-500"}`}
                            >
                                {tc.category}
                            </span>
                        </button>
                    );
                })}
            </div>
        </div>
    );
};
