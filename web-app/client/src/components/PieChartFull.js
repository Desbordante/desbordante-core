/* eslint-disable */

import React, { useState, useEffect } from "react";
import "./PieChartFull.css";
import SearchBar from "./SearchBar";
import { Doughnut, Pie } from "react-chartjs-2";
import Button from "./Button";

function PieChartFull({
  title,
  attributes,
  maxItemsShown = 9,
  maxItemsSelected = 10,
  colors,
}) {
  // Get how much px is one rem, later used in chart dimensions
  const rem = parseFloat(getComputedStyle(document.documentElement).fontSize);

  // Pre-defined colors
  // const colors = [
  //   "#ff5757",
  //   "#575fff",
  //   "#4de3a2",
  //   "#edc645",
  //   "#d159de",
  //   "#32bbc2",
  //   "#ffa857",
  //   "#8dd44a",
  //   "#6298d1",
  // ];

  const [searchString, setSearchString] = useState("");
  const [selectedAttributes, setSelectedAttributes] = useState([]);
  const [foundAttributes, setFoundAttributes] = useState([]);
  const [depth, setDepth] = useState(0);
  const [otherValue, setOtherValue] = useState(0);
  const [displayAttributes, setDisplayAttributes] = useState([]);

  // console.log(foundAttributes);
  // console.log(depth);
  // console.log(displayAttributes);

  // Update found attributes if search string changes or attributes change. Keep found attributes sorted.
  useEffect(() => {
    const newFoundAttributes = searchString
      ? attributes.filter((attr) => attr.name.includes(searchString))
      : attributes
      ? attributes
      : [];

    setFoundAttributes(
      newFoundAttributes
        .filter((attr) => !selectedAttributes.includes(attr))
        .sort((a, b) => b.value - a.value)
    );
  }, [attributes, searchString, selectedAttributes]);

  // Set DisplayAttributes to top-{maxItemsShown} of found attributes. Add the "Other" value, if needed.
  useEffect(() => {
    let newDisplayAttributes = foundAttributes.slice(
      maxItemsShown * depth,
      maxItemsShown * (depth + 1)
    );

    let newOtherValue = 0;
    foundAttributes
      .slice(maxItemsShown * (depth + 1))
      .forEach((attr) => (newOtherValue += attr.value));

    if (foundAttributes.length > maxItemsShown * (depth + 1)) {
      newDisplayAttributes.push({ name: "Other", value: newOtherValue });
    }

    setDisplayAttributes(newDisplayAttributes);
  }, [foundAttributes, foundAttributes, depth]);

  // console.log(selectedAttributes);

  return (
    <div className="pie-chart-full">
      <h1 className="chart-title">{title}</h1>
      <SearchBar
        defaultText="Filter attributes..."
        setSearchString={setSearchString}
        onClick={() => setDepth(depth === 0 ? 0 : depth - 1)}
      />
      <div className="chart">
        <Doughnut
          className="chart-canvas"
          // width={30 * rem}
          // height={30 * rem}
          data={{
            labels: displayAttributes.map((attr) => attr.name),
            datasets: [
              {
                data: displayAttributes.map((attr) => attr.value),
                backgroundColor: colors,
                borderColor: "#ffffff",
                hoverBorderColor: "#ffffff",
                borderWidth: 0.2 * rem,
                hoverOffset: 1 * rem,
                // borderAlign: "inner",
              },
            ],
          }}
          options={{
            onClick: (event, item) => {
              if (item.length > 0) {
                if (item[0].index == maxItemsShown) {
                  setDepth(depth + 1);
                } else {
                  setSelectedAttributes(
                    selectedAttributes
                      .concat(
                        item.length ? [displayAttributes[item[0].index]] : []
                      )
                      .slice(0, maxItemsSelected)
                  );
                }
              }
            },
            maintainAspectRatio: false,
            cutout: "50%",
            cutoutPercentage: 10,
            layout: {
              padding: 1 * rem,
            },
            plugins: {
              legend: {
                // display: false,
                position: "left",
                onClick: () => {}, // TODO: do something useful on label click
                labels: {
                  color: "#000000",
                  font: {
                    family: "'Roboto', sans-serif",
                    size: 1 * rem,
                    weight: "300",
                  },
                  boxWidth: 1 * rem,
                  boxHeight: 1 * rem,
                },
              },
              tooltip: {
                displayColors: false,
                cornerRadius: 1 * rem,
                backgroundColor: "#e5e5e5",
                // borderColor: "#7600d1",
                // borderWidth: 0.2 * rem,
                titleColor: "#000000",
                titleAlign: "center",
                titleFont: {
                  family: "'Roboto', sans-serif",
                  size: 1 * rem,
                  weight: "600",
                },
                bodyColor: "#000000",
                bodyAlign: "center",
                bodyFont: {
                  family: "'Roboto', sans-serif",
                  size: 1 * rem,
                  weight: "400",
                },
                titleMarginBottom: 0.5 * rem,
                padding: 1 * rem,
                callbacks: {
                  // title: (tooltipItem) => "" + tooltipItem[0].label,
                  // label: (tooltipItem) => "" + tooltipItem.formattedValue,
                  label: (tooltipItem) => "" + tooltipItem.label,
                },
              },
            },
            animation: {
              animateRotate: false,
            },
          }}
        />
      </div>
      <div className="selected-attributes">
        {selectedAttributes.map((attr, index) => (
          <Button
            onClick={() =>
              setSelectedAttributes(
                selectedAttributes.filter((_, idx) => index != idx)
              )
            }
            key={index}
            text={attr.name}
            color="black"
          />
        ))}
      </div>
    </div>
  );
}

export default PieChartFull;
