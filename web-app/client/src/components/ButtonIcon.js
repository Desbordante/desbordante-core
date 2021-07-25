import React from "react";
import "./ButtonIcon.css";

function Button({ src, alt, onClick, color }) {
  return (
    <button
      type="button"
      className={"round-corners button button-icon checked " + color}
      onClick={onClick}
    >
      <img src={src} alt={alt} className="icon" />
    </button>
  );
}

export default Button;
