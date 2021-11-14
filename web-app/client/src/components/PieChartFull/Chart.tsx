import React from "react";
import { Doughnut } from "react-chartjs-2";
import { coloredAttribute } from "../../types";

import AttributeLabel from "../AttributeLabel/AttributeLabel";
import SelectedAttribute from "../SelectedAttribute/SelectedAttribute";
import "./PieChartFull.scss";

/* eslint-disable no-unused-vars */
interface Props {
  displayAttributes: coloredAttribute[];
  onSelect: (a: any, b: any) => void;
  selectedAttributes: coloredAttribute[];
  setSelectedAttributes: (attr: coloredAttribute[]) => void;
}
/* eslint-enable no-unused-vars */

const Chart: React.FC<Props> = ({
  displayAttributes,
  onSelect,
  selectedAttributes,
  setSelectedAttributes,
}) => {
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

  return (
    <div className="chart">
      <div className="legend-and-canvas">
        <div className="chart-legend">
          {displayAttributes.map((attr, index) => (
            <AttributeLabel
              text={attr.name}
              labelColor={colors[index]}
              key={attr.name}
            />
          ))}
        </div>
        <div className="chart-canvas">
          <Doughnut
            style={{
              position: "absolute",
              backgroundColor: "#00000000",
            }}
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
                },
              ],
            }}
            options={{
              onClick: onSelect,
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
                    label: (tooltipItem: any) => tooltipItem.label,
                  },
                },
              },
              animation: {
                animateRotate: false,
              },
            }}
          />
        </div>
      </div>
      <div className="selected-attributes">
        {selectedAttributes.map((attr, index) => (
          <SelectedAttribute
            onDelete={() => {
              setSelectedAttributes(
                /* eslint-disable-next-line comma-dangle */
                selectedAttributes.filter((_, idx) => index !== idx)
              );
            }}
            key={attr.name}
            text={attr.name}
          />
        ))}
      </div>
    </div>
  );
};

export default Chart;
