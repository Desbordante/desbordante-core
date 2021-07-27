import React from "react";
import "./ButtonIcon.css";

function ButtonIcon({
  src,
  alt,
  onClick,
  size = "normal",
  glow = false,
  color,
  style,
}) {
  return (
    <button
      type="button"
      className={`round-corners button button-icon ${color} ${
        glow ? "glowing" : ""
      } ${size}`}
      style={style}
      onClick={onClick}
    >
      <img src={src} alt={alt} className={`icon ${size}`} />
    </button>
  );
}

export default ButtonIcon;
