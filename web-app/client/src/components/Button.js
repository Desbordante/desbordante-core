import React from "react";
import "./Button.css";

function Button({
  src,
  alt,
  onClick,
  size = "normal",
  glow = false,
  color,
  style,
  icon = false,
  text,
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
      {icon ? <img src={src} alt={alt} className={`icon ${size}`} /> : text}
    </button>
  );
}

export default Button;
