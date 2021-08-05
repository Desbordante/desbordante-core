import React from "react";
import ReactDOM from "react-dom";
import DependencyListFull from "../DependencyListFull";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<DependencyListFull />, div);
});

