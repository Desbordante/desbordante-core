import React from "react";
import ReactDOM from "react-dom";
import "jest-canvas-mock";

import "./mocks";
import App from "./App.tsx.old";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<App />, div);
  ReactDOM.unmountComponentAtNode(div);
});
