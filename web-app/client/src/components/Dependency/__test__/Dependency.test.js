import React from "react";
import ReactDOM from "react-dom";
import Dependency from "../Dependency";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<Dependency />, div);
});
