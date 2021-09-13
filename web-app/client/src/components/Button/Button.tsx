import React, { useState } from "react";
import "./Button.css";

interface Props {
  type: "button" | "submit";
  onClick: React.MouseEventHandler<HTMLButtonElement>;
  enabled?: boolean;
  color?: "0" | "1" | "gradient" | "error";
  glow?: "no" | "hover" | "always";
  glowRadius?: number;
  size?: number;
}

const Button: React.FC<Props> = ({
  type,
  onClick,
  enabled = true,
  color = "0",
  glow = "hover",
  glowRadius = 0.4,
  children,
  size = 3,
}) => {
  const [hover, setHover] = useState(false);

  return (
    <div
      onMouseEnter={() => setHover(true)}
      onMouseLeave={() => setHover(false)}
      className="button-container"
    >
      {glow !== "no" && enabled && (
        <div
          className={`glow color-${color}`}
          style={{
            filter: `blur(${glow !== "hover" || hover ? glowRadius : 0}rem)`,
          }}
        >
          {children}
        </div>
      )}
      <button
        type={type}
        className={`${enabled ? "" : "disabled"} color-${color}`}
        style={{ fontSize: `${size * 0.4}rem`, padding: `${size * 0.25}rem ${size * 0.5}rem` }}
        onClick={enabled ? onClick : () => {}}
      >
        {children}
      </button>
    </div>
  );
};

export default Button;
