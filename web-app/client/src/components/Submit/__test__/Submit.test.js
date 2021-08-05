import React from "react";
import ReactDOM from "react-dom";
import Submit from "../Submit";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<Submit />, div);
});

