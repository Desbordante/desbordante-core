import React from "react";
import "./StatusDisplay.scss";

interface Props {
  text: string;
  type: "loading" | "error";
}

const StatusDisplay: React.FC<Props> = ({ text, type }) => (
  <div className="status-display">
    {text}
    <img
      src={type === "loading" ? "/images/loading.gif" : ""} // TODO: Error icon
      alt=""
    />
  </div>
);

export default StatusDisplay;
