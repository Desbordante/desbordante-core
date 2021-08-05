import React from "react";
import ReactDOM from "react-dom";
import SelectedAttributes from "../SelectedAttributes";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<SelectedAttributes />, div);
});

