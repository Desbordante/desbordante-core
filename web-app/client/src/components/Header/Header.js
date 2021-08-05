import React from "react";
import "./Header.css";

function Header() {
  return (
    <>
      <div className="video-header">
        <video autoPlay loop muted id="video">
          <source src="/videos/background.mp4" type="video/mp4" />
        </video>
        <div className="name-and-logo">
          <img src="/icons/logo.svg" className="logo-big" alt="logo" />
          <h1 className="name-main">Desbordante</h1>
        </div>
        <h2 className="description">
          An
          {" "}
          <a
            href="https://github.com/Mstrutov/Desbordante"
            rel="noreferrer"
            target="_blank"
          >
            open-source
          </a>
          {" "}
          data profiling tool
        </h2>
      </div>
      <div className="bg-gradient" />
    </>
  );
}

export default Header;
