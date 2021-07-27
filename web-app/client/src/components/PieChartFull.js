/* eslint-disable */

import React, { useState, useEffect } from "react";
import "./PieChartFull.css";
import SearchBar from "./SearchBar";
import { Doughnut, Pie } from "react-chartjs-2";
import ButtonIcon from "./ButtonIcon";

function PieChartFull({
  title,
  attributes,
  maxItemsShown = 9,
  maxItemsSelected = 4,
}) {
  const rem = parseFloat(getComputedStyle(document.documentElement).fontSize);

  let colors = [...Array(maxItemsShown)]
    .map((_, index) => `hsla(${parseInt((index * 360) / 10)}, 75%, 50%, 0.7)`)
    .sort(() => 0.5 - Math.random());
  colors.push("hsla(0, 0%, 50%, 0.7)");

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
  const [selected, setSelected] = useState([]);

  const [foundAttributes, setFoundAttributes] = useState([]);

  const [depth, setDepth] = useState(0);
  const [otherValue, setOtherValue] = useState(0);

  const [displayAttributes, setDisplayAttributes] = useState([]);

  useEffect(() => {
    const newFoundAttributes = searchString
      ? attributes.filter((attr) => attr.name.includes(searchString))
      : attributes
      ? attributes
      : [];

    // console.log(searchString);
    // console.log(newFoundAttributes);

    setFoundAttributes(newFoundAttributes.sort((a, b) => b.value - a.value));
  }, [attributes, searchString]);

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
  }, [foundAttributes, foundAttributes]);

  return (
    <div className="pie-chart-full">
      <h1 className="chart-title">{title}</h1>
      <SearchBar
        defaultText="Filter attributes..."
        setSearchString={setSearchString}
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
            maintainAspectRatio: false,
            cutout: "50%",
            cutoutPercentage: 10,
            layout: {
              padding: 1 * rem,
            },
            plugins: {
              legend: {
                display: false,
                position: "bottom",
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
            // animation: {
            //   animateRotate: false,
            // },
          }}
        />
        <ButtonIcon
          src="/icons/upload.svg"
          alt="Q"
          color="green"
          size="huge"
          style={{}}
          onClick={() => console.log("CLICKED")}
          style={{
            position: "relative",
            // zIndex: 1,
            transform: "translate(calc(17.5vw - 3.8rem), calc(15rem - 2.8rem))",
          }}
        />
      </div>
    </div>
  );
}

export default PieChartFull;
