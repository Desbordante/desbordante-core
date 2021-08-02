import React from "react";
import "./NavButton.css";

function NavButton({ src, alt, onClick }) {
  return (
    <div className="nav-button">
      <button type="button" className="nav glow">
        <img src={src} alt={alt} />
      </button>

      <button type="button" className="nav" onClick={onClick}>
        <img src={src} alt={alt} />
      </button>
    </div>
  );
}

export default NavButton;
