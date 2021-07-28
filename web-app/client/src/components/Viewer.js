/* eslint-disable */

import React, { useState, useEffect } from "react";
import "./Viewer.css";
import PieChartFull from "./PieChartFull";

function Viewer() {
  // Faking attributes update over time
  const [attributes, setAttributes] = useState(
    [...Array(20)].map((value, index) => ({
      name: `Attr_${index}`,
      value: index + 1,
    }))
  );

  // let a = 0;
  // useEffect(() => {
  //   // if (a === 0) {
  //   setAttributes(
  //     attributes.map((attr) => ({ name: attr.name, value: attr.value + 1 }))
  //   );
  //   // }
  //   a++;
  //   a %= 1000;
  // }, [attributes]);

  // console.log(attributes);

  return (
    <div className="bg-light">
      <PieChartFull title="Left-hand side" attributes={attributes} />
      <PieChartFull title="Right-hand side" attributes={attributes} />
    </div>
  );
}

export default Viewer;
