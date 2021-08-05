import React from "react";
import "./OnscreenMessage.css";

function OnscreenMessage({ text, style }) {
  return (
    <div className="onscreen-message" style={style}>
      {text}
      <img src="/images/loading-animation.gif" alt="Please, wait." />
    </div>
  );
}

export default OnscreenMessage;
