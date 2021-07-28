import React from "react";
import "./LoadingScreen.css";
import ProgressBar from "./ProgressBar";
import Button from "./Button";

function LoadingScreen({ onComplete, progress, onCancel }) {
  if (progress === 1) {
    onComplete();
  }

  return (
    <div className="message-and-bar">
      <h1>Uploading your file. Please, wait.</h1>
      <ProgressBar
        maxWidth={50}
        widthUnit="rem"
        progress={progress}
        thickness={0.8}
        rounded
      />
      <Button text="Cancel" color="green" onClick={onCancel} />
    </div>
  );
}

export default LoadingScreen;
