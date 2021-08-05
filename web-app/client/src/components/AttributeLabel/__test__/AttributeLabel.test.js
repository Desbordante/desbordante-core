import React from "react";
import ReactDOM from "react-dom";
import AttributeLabel from "../AttributeLabel";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<AttributeLabel />, div);
});
