import React from "react";
import "./Header.css";

function Header() {
  return (
    <>
      <div className="video-header">
        <video autoPlay loop muted id="video">
          <source src="/videos/background.mp4" type="video/mp4"></source>
        </video>
        <div className="name-and-logo">
          <img src="/icons/logo.svg" className="logo-big" alt="logo" />
          <h1 className="name-main">Desbordante</h1>
        </div>
        <h2 className="description">Open-source data profiling tool</h2>
      </div>
      <div className="bg-gradient"></div>
    </>
  );
}

export default Header;
