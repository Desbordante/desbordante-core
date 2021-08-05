import React from "react";
import ReactDOM from "react-dom";
import LoadingScreen from "../LoadingScreen";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<LoadingScreen />, div);
});

