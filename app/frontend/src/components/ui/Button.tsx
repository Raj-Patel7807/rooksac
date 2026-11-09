import React from "react";

export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
    variant?: "primary" | "secondary" | "danger" | "outline";
    size?: "sm" | "md" | "lg";
}

export const Button: React.FC<ButtonProps> = ({
    children,
    variant = "primary",
    size = "md",
    className = "",
    ...props
}) => {
    const base =
        "inline-flex items-center justify-center font-medium rounded transition focus:outline-none focus:ring-2 disabled:opacity-50 disabled:cursor-not-allowed";

    const variants = {
        primary: "bg-blue-600 text-white hover:bg-blue-700 focus:ring-blue-500",
        secondary:
            "bg-zinc-700 text-zinc-100 hover:bg-zinc-600 focus:ring-zinc-400",
        danger: "bg-red-600 text-white hover:bg-red-700 focus:ring-red-500",
        outline:
            "border border-zinc-600 text-zinc-200 hover:bg-zinc-800 focus:ring-zinc-500",
    };

    const sizes = {
        sm: "px-2.5 py-1 text-xs",
        md: "px-3.5 py-1.5 text-sm",
        lg: "px-5 py-2.5 text-base",
    };

    return (
        <button
            className={`${base} ${variants[variant]} ${sizes[size]} ${className}`}
            {...props}
        >
            {children}
        </button>
    );
};
