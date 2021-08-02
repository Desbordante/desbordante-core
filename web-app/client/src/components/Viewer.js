/* eslint-disable */

import React, { useState, useEffect, useRef } from "react";
import "./Viewer.css";
import PieChartFull from "./PieChartFull";
import NavButton from "./NavButton";
import DependencyListFull from "./DependencyListFull";

function Viewer({ currentState }) {
  const [state, setState] = useState(0);

  // Faking attributes update over time
  const [attributes, setAttributes] = useState(
    [
      "lawyer",
      "offender",
      "stereotype",
      "host",
      "plot",
      "certain",
      "panic",
      "spill",
      "tumour",
      "wedding",
      "deprive",
      "tax",
      "insistence",
      "civilian",
      "qualified",
      "robot",
      "reconcile",
      "virus",
      "still",
      "refrigerator",
    ].map((value) => ({
      name: value,
      value: parseInt(Math.random() * 100),
    }))
  );

  const [selectedAttributesLHS, setSelectedAttributesLHS] = useState([]);
  const [selectedAttributesRHS, setSelectedAttributesRHS] = useState([]);

  const [dependencies, setDependencies] = useState(
    [...Array(40)].map(() => ({
      lhs: [...Array(parseInt(Math.random() * 7) + 1)]
        .map(() => parseInt(Math.random() * 20))
        .map((attrNumber) => attributes[attrNumber].name)
        .sort((attr1, attr2) => attr1.localeCompare(attr2)),
      rhs: attributes[parseInt(Math.random() * 20)].name,
    }))
  );

  const attributePart = useRef();
  const dependencyPart = useRef();

  useEffect(() => {
    [attributePart, dependencyPart][state].current.scrollIntoView({
      behavior: "smooth",
      block: "end",
    });
  }, [state]);

  // Number of slices shown in charts (+1 for "Other")
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

  return (
    <>
      <div className="bg-light" ref={attributePart}>
        <div className="charts-with-controls">
          <PieChartFull
            title="Left-hand side"
            attributes={attributes}
            colors={colors}
            selectedAttributes={selectedAttributesLHS}
            setSelectedAttributes={setSelectedAttributesLHS}
          />
          <PieChartFull
            title="Right-hand side"
            attributes={attributes}
            maxItemsSelected={1}
            colors={colors}
            selectedAttributes={selectedAttributesRHS}
            setSelectedAttributes={setSelectedAttributesRHS}
          />
        </div>
        <footer>
          <h1
            className="bottom-title"
            style={{ color: "#000000", fontWeight: 500 }}
          >
            View Dependencies
          </h1>
          <NavButton
            src="/icons/nav-down.svg"
            alt="down"
            onClick={() => setState(1)}
          />
        </footer>
      </div>
      <div
        className="bg-light"
        ref={dependencyPart}
        style={{ justifyContent: "space-between" }}
      >
        <DependencyListFull
          dependencies={dependencies}
          attributes={attributes}
        />
        <footer>
          <h1
            className="bottom-title"
            style={{ color: "#000", fontWeight: 500 }}
          >
            View Attributes
          </h1>
          <NavButton
            src="/icons/nav-up.svg"
            alt="up"
            onClick={() => setState(0)}
          />
        </footer>
      </div>
    </>
  );
}

export default Viewer;
