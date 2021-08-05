import React from "react";
import ReactDOM from "react-dom";
import Radio from "../Radio";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<Radio />, div);
});

