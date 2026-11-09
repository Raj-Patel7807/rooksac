import React from "react";

export interface CardProps extends React.HTMLAttributes<HTMLDivElement> {
    title?: string;
}

export const Card: React.FC<CardProps> = ({
    title,
    children,
    className = "",
    ...props
}) => {
    return (
        <div
            className={`bg-zinc-900 border border-zinc-800 rounded-lg p-4 shadow-sm text-zinc-100 ${className}`}
            {...props}
        >
            {title && (
                <h3 className="text-md font-semibold mb-3 border-b border-zinc-800 pb-2">
                    {title}
                </h3>
            )}
            {children}
        </div>
    );
};
