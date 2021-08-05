/* eslint-disable */

import React, { useState, useEffect } from "react";
import "./PieChartFull.css";
import SearchBar from "../SearchBar/SearchBar";
import { Doughnut } from "react-chartjs-2";
import Button from "../Button/Button";
import SelectedAttribute from "../SelectedAttribute/SelectedAttribute";
import AttributeLabel from "../AttributeLabel/AttributeLabel";

function PieChartFull({
  title = "Chart title",
  attributes = { lhs: [], rhs: [] },
  maxItemsShown = 9,
  maxItemsSelected = 9,
  selectedAttributes = [],
  setSelectedAttributes,
}) {
  // Get how much px is one rem, later used in chart dimensions
  const rem = parseFloat(getComputedStyle(document.documentElement).fontSize);

  // Pre-defined colors
  const colors = [
    "#ff5757",
    "#575fff",
    "#4de3a2",
    "#edc645",
    "#d159de",
    "#32bbc2",
    "#ffa857",
    "#8dd44a",
    "#6298d1",
    "#969696",
  ];

  const [searchString, setSearchString] = useState("");
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
      <h1 className="title">{title}</h1>
      <SearchBar
        defaultText="Filter attributes..."
        setSearchString={setSearchString}
      />
      <div className="chart">
        <div className="chart-legend">
          {displayAttributes.map((attr, index) => (
            <AttributeLabel
              text={attr.name}
              labelColor={colors[index]}
              key={index}
            />
          ))}
        </div>
        <div className="chart-canvas">
          <Doughnut
            style={{
              position: "absolute",
              backgroundColor: "#00000000",
            }}
            // width={100}
            // height={100}
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
              responsive: true,
              cutout: "50%",
              cutoutPercentage: 10,
              layout: {
                padding: 1 * rem,
              },
              plugins: {
                legend: {
                  display: false,
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
          <Button
            src="/icons/up-depth.svg"
            alt="Search"
            color="purple"
            onClick={() => setDepth(depth === 0 ? 0 : depth - 1)}
            size={4}
            sizeUnit="vh"
            icon
            style={{
              position: "absolute",
              zIndex: 1,
              padding: "2rem",
              opacity: depth > 0 ? 1 : 0,
              cursor: depth > 0 ? "pointer" : "default"
            }}
          />
        </div>
      </div>
      <div className="selected-attributes">
        {selectedAttributes.map((attr, index) => (
          <SelectedAttribute
            onDelete={() =>
              setSelectedAttributes(
                selectedAttributes.filter((_, idx) => index != idx)
              )
            }
            key={index}
            text={attr.name}
          />
        ))}
      </div>
    </div>
  );
}

export default PieChartFull;
