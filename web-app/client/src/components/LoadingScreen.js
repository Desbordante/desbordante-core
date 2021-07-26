import React, { useState, useEffect } from "react";
import "./LoadingScreen.css";
import ProgressBar from "./ProgressBar";

function LoadingScreen({ onComplete, state }) {
  const [percent, setPercent] = useState(0);
  // eslint-disable-next-line no-unused-vars
  const [complete, setComplete] = useState(false);

  useEffect(() => {
    if (percent >= 1) {
      setComplete(true);
      onComplete();
    } else {
      setPercent(state === 0 ? 0 : percent + 0.0001);
    }
  });

  return (
    <div className="message-and-bar">
      <h1>Uploading your file. Please, wait.</h1>
      <ProgressBar
        maxWidth={50}
        widthUnit="rem"
        percent={percent}
        thickness={0.8}
        rounded
      />
    </div>
  );
}

export default LoadingScreen;
