import React from "react";
import ReactDOM from "react-dom";
import Checkbox from "../Toggle";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<Checkbox />, div);
});
