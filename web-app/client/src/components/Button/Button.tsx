import React from "react";
import "./Button.scss";

interface Props {
  type: "button" | "submit";
  onClick: React.MouseEventHandler<HTMLButtonElement>;
  enabled?: boolean;
  color?: "0" | "1" | "error";
  size?: number;
}

const Button: React.FC<Props> = ({
  type,
  onClick,
  enabled = true,
  color = "0",
  children,
  size = 3,
}) => (
  <button
    type={type}
    className={`button ${enabled ? "" : "disabled"} color-${color}`}
    style={{
      fontSize: `${size * 0.4}rem`,
      padding: `${size * 0.25}rem ${size * 0.5}rem`,
    }}
    onClick={enabled ? onClick : () => {}}
  >
    {children}
  </button>
);

export default Button;
