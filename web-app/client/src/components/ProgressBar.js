import React from "react";
import "./ProgressBar.css";

function ProgressBar({
  maxWidth, widthUnit, thickness, percent, rounded,
}) {
  const barWidth = maxWidth * percent;

  const styles = {
    width: maxWidth + widthUnit,
    height: `${thickness}rem`,
    borderRadius: rounded ? `${1000}px` : 0,
  };

  return (
    // <div className="progress-bar">
    <div className="progress-bar" style={styles}>
      <div className="progress-bg" style={styles} />
      <div
        className="progress-bg progress-accent glow"
        style={{ ...styles, width: barWidth + widthUnit, opacity: `${50}%` }}
      />
      <div
        className="progress-bg progress-accent"
        style={{ ...styles, width: barWidth + widthUnit }}
      />
    </div>
    // </div>
  );
}

export default ProgressBar;
