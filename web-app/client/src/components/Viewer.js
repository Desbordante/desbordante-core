import React from "react";
import "./Viewer.css";
import PieChartFull from "./PieChartFull";

function Viewer() {
  return (
    <div className="bg-light">
      <PieChartFull title="Left-hand side" />
      <PieChartFull title="Right-hand side" />
    </div>
  );
}

export default Viewer;
