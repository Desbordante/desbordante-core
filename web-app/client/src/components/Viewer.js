/* eslint-disable */

import React, { useState } from "react";
import "./Viewer.css";
import PieChartFull from "./PieChartFull";

function Viewer() {
  const [attributes, setAttributes] = useState(
    [...Array(40)].map((value, index) => ({
      name: `Attr_${index}`,
      value: Math.pow(index, 8),
    }))
  );

  // console.log(attributes);

  return (
    <div className="bg-light">
      <PieChartFull title="Left-hand side" attributes={attributes} />
      <PieChartFull title="Right-hand side" attributes={attributes} />
    </div>
  );
}

export default Viewer;
