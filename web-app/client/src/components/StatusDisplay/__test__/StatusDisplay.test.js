import React from "react";
import ReactDOM from "react-dom";
import OnScreenMessage from "../StatusDisplay";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<OnScreenMessage />, div);
});
