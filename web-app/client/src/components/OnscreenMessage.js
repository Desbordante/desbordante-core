import React from "react";
import "./OnscreenMessage.css";

function OnscreenMessage({ text }) {
  return (
    <div className="onscreen-message">
      {text}
    </div>
  );
}

export default OnscreenMessage;
