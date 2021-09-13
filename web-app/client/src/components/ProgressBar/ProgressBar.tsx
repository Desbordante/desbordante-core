import React from "react";
import "./ProgressBar.css";

interface Props {
  progress: number;
  maxWidth?: number;
  thickness?: number;
  rounded?: boolean;
}

const ProgressBar: React.FC<Props> = ({
  progress,
  maxWidth = 100,
  thickness = 5,
  rounded = true,
}) => {
  const barWidth = maxWidth * progress;

  const style = {
    width: `${maxWidth}%`,
    height: `${thickness}rem`,
    borderRadius: rounded ? "1000px" : 0,
  };

  return (
    <div className="progress-bar" style={style}>
      <div className="progress-bg" style={style} />
      <div
        className="progress-bg progress-accent"
        style={{
          ...style,
          width: `${barWidth}%`,
          opacity: "50%",
          filter: "blur(2rem)",
        }}
      />
      <div
        className="progress-bg progress-accent"
        style={{ ...style, width: `${barWidth}%` }}
      />
    </div>
  );
};

export default ProgressBar;
