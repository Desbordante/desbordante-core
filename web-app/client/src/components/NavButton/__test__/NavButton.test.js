import React from "react";
import ReactDOM from "react-dom";
import NavButton from "../NavButton";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<NavButton />, div);
});

