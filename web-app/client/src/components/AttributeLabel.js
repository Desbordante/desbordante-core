import React from "react";
import "./AttributeLabel.css";

function ChartLegend({ text, labelColor }) {
  return (
    <div className="attribute-label">
      <div className="circle" style={{ backgroundColor: labelColor }} />
      {text}
    </div>
  );
}

export default ChartLegend;
