import React from "react";
import ReactDOM from "react-dom";
import PieChartFull from "../PieChartFull";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<PieChartFull />, div);
});

