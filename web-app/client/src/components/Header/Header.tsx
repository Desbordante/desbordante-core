/* eslint-disable react/jsx-one-expression-per-line */

import React from "react";
import "./Header.scss";

const Header: React.FC = () => (
  <>
    <div className="video-header">
      <video autoPlay loop muted>
        <source src="/videos/background-min-cropped.webm" type="video/webm" />
      </video>
      <div className="name-and-logo">
        <img src="/icons/logo.svg" className="logo-big" alt="logo" />
        <h1 className="name-main">Desbordante</h1>
      </div>
      <h2 className="description">
        An{" "}
        <a
          href="https://github.com/Mstrutov/Desbordante"
          rel="noreferrer"
          target="_blank"
        >
          open-source
        </a>{" "}
        data profiling tool
      </h2>
    </div>
    <div className="bg-gradient" />
  </>
);

export default Header;
