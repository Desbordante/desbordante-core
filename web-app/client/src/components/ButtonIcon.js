import React from "react";
import "./ButtonIcon.css";

function Button({
  src, alt, onClick,
}) {
  return (
    <button
      type="button"
      className="round-corners button button-icon checked purple"
      onClick={onClick}
    >
      <img src={src} alt={alt} className="icon" />
    </button>
  );
}

export default Button;
