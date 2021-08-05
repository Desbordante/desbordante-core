/* eslint-disable */

import React from "react";
import "./Dependency.css";

function Dependency({ lhs = [], rhs = "", isActive = false, onClick }) {
  return (
    <div className="dependency" onClick={onClick}>
      {lhs.map((attr, index) => (
        <div
          className={`attribute-name ${isActive ? "active" : ""}`}
          key={index}
        >
          {attr}
        </div>
      ))}
      <svg viewBox="0 0 58.73 20.09" className={isActive ? "active" : ""}>
        <line x1="48.68" y1="0.5" x2="58.23" y2="10.05" />
        <line x1="58.23" y1="10.05" x2="48.68" y2="19.59" />
        <line x1="58.23" y1="10.05" x2="0.5" y2="10.05" />
      </svg>
      <div className={`attribute-name ${isActive ? "active" : ""}`}>{rhs}</div>
    </div>
  );
}

export default Dependency;
