import React from "react";
import ReactDOM from "react-dom";
import DependeencyListFull from "../DependeencyListFull";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<DependeencyListFull />, div);
});

