import React from "react";
import "./Toggle.scss";

interface Props {
  toggleCondition: boolean;
  onClick: React.MouseEventHandler<HTMLButtonElement>;
  isEnabled?: boolean;
  color?: "0" | "1";
}

const Toggle: React.FC<Props> = ({
  toggleCondition,
  onClick,
  isEnabled = true,
  color = "0",
  children,
}) => (
  <button
    type="button"
    className={`toggle ${
      isEnabled && toggleCondition ? "enabled" : ""
    } color-${color}`}
    style={{}}
    onClick={isEnabled ? onClick : () => {}}
  >
    {children}
  </button>
);

export default Toggle;
