import React from "react";
import ReactDOM from "react-dom";
import SelectedAttribute from "../SelectedAttribute";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<SelectedAttribute />, div);
});

