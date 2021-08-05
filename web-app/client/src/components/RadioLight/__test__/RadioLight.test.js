import React from "react";
import ReactDOM from "react-dom";
import RadioLight from "../RadioLight";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<RadioLight />, div);
});

