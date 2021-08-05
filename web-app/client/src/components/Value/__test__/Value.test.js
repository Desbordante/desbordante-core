import React from "react";
import ReactDOM from "react-dom";
import Value from "../Value";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<Value />, div);
});

