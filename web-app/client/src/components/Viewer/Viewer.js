/* eslint-disable */

import React, { useState, useEffect, useRef } from "react";
import "./Viewer.css";
import PieChartFull from "../PieChartFull/PieChartFull";
import NavButton from "../NavButton/NavButton";
import DependencyListFull from "../DependencyListFull/DependencyListFull";
import OnscreenMessage from "../OnscreenMessage/OnscreenMessage";

function Viewer({
  currentState,
  attributesLHS,
  attributesRHS,
  dependencies,
  taskFinished,
  taskStatus,
}) {
  const [state, setState] = useState(0);

  const [selectedAttributesRHS, setSelectedAttributesRHS] = useState([]);
  const [selectedAttributesLHS, setSelectedAttributesLHS] = useState([]);

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

  // // Chart colors, evenly distributed on the color wheel
  // const startColor = parseInt(Math.random() * 360);
  // let colors = [...Array(maxItemsShown)]
  //   .map(
  //     (_, index) =>
  //       `hsla(${parseInt(startColor + (index * 360) / 10) %
  //         360}, 75%, 50%, 0.7)`
  //   )
  //   .sort(() => 0.5 - Math.random());

  // // Grey color for "Other" label
  // colors.push("hsla(0, 0%, 50%, 0.7)");

  return (
    <>
      <div className="bg-light" ref={attributePart}>
        {taskFinished ? null : (
          <OnscreenMessage
            text="Loading"
            // style={{
            //   opacity: taskFinished ? 0 : 1,
            // }}
          />
        )}
        <div
          className="charts-with-controls"
          style={{
            opacity: taskFinished ? 1 : 0,
            zIndex: taskFinished ? 1000 : 0,
          }}
        >
          <PieChartFull
            title="Left-hand side"
            attributes={attributesLHS}
            selectedAttributes={selectedAttributesLHS}
            setSelectedAttributes={setSelectedAttributesLHS}
          />
          <PieChartFull
            title="Right-hand side"
            attributes={attributesRHS}
            maxItemsSelected={1}
            selectedAttributes={selectedAttributesRHS}
            setSelectedAttributes={setSelectedAttributesRHS}
          />
        </div>
        <footer style={{ opacity: taskFinished ? 1 : 0 }}>
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
        </footer>{" "}
      </div>
      <div
        className="bg-light"
        ref={dependencyPart}
        style={{ justifyContent: "space-between" }}
      >
        <DependencyListFull
          dependencies={dependencies}
          selectedAttributesLHS={selectedAttributesLHS}
          selectedAttributesRHS={selectedAttributesRHS}
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
