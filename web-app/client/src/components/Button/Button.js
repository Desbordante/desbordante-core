import React from "react";
import "./Button.css";

function Button({
  src,
  alt,
  onClick,
  size = 1.4,
  glow = false,
  color,
  icon = false,
  text,
  style,
  sizeUnit = "rem",
}) {
  return (
    <button
      type="button"
      className={`round-corners button ${icon ? "button-icon" : ""} ${color} ${
        glow ? "glowing" : ""
      } ${size}`}
      onClick={onClick}
      style={style}
    >
      {icon ? (
        <img
          src={src}
          alt={alt}
          style={{ height: size + sizeUnit, width: size + sizeUnit }}
        />
      ) : (
        text
      )}
    </button>
  );
}

export default Button;
