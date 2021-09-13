import React from "react";
import "./Toggle.css";

interface Props {
  toggleCondition: boolean;
  onClick: React.MouseEventHandler<HTMLButtonElement>;
  isEnabled?: boolean;
  color?: "0" | "1" | "gradient";
  glow?: "no" | "toggle";
  glowRadius?: number;
}

const Toggle: React.FC<Props> = ({
  toggleCondition,
  onClick,
  isEnabled = true,
  color = "0",
  glow = "toggle",
  glowRadius = 0.4,
  children,
}) => (
  <div className="toggle-container">
    {glow !== "no" && isEnabled && (
      <div
        className={`glow ${toggleCondition ? "enabled" : ""} color-${color}`}
        style={{
          filter: `blur(${toggleCondition ? glowRadius : 0}rem)`,
        }}
      />
    )}
    <button
      type="button"
      className={`${
        isEnabled && toggleCondition ? "enabled" : ""
      } color-${color}`}
      style={{}}
      onClick={isEnabled ? onClick : () => {}}
    >
      {children}
    </button>
  </div>
);

export default Toggle;
