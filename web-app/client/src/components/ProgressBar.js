import React from "react";
import "./ProgressBar.css";

function ProgressBar({
  maxWidth, widthUnit, thickness, progress, rounded,
}) {
  const barWidth = maxWidth * progress;

  const styles = {
    width: maxWidth + widthUnit,
    height: `${thickness}rem`,
    borderRadius: rounded ? `${1000}px` : 0,
  };

  return (
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
  );
}

export default ProgressBar;
