/* eslint-disable */

import React, { useState, useEffect } from "react";
import "./Viewer.css";
import PieChartFull from "./PieChartFull";

function Viewer() {
  // Faking attributes update over time
  const [attributes, setAttributes] = useState(
    [...Array(20)].map((value, index) => ({
      name: `Attr_${index}`,
      value: index *2 + 1,
    }))
  );

  const maxItemsShown = 9;

  // Chart colors, evenly distributed on the color wheel
  const startColor = parseInt(Math.random() * 360);
  let colors = [...Array(maxItemsShown)]
    .map(
      (_, index) =>
        `hsla(${parseInt(startColor + (index * 360) / 10) %
          360}, 75%, 50%, 0.7)`
    )
    .sort(() => 0.5 - Math.random());

  // Grey color for "Other" label
  colors.push("hsla(0, 0%, 50%, 0.7)");

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
      <div className="charts">
        <PieChartFull title="Left-hand side" attributes={attributes} colors={colors}/>
        <PieChartFull title="Right-hand side" attributes={attributes} maxItemsSelected={1} colors={colors}/>
      </div>
      {/* <h1 className="bottom-title" style={{color: "#000", fontWeight: 500}}>View Dependencies</h1> */}
    </div>
  );
}

export default Viewer;
