import React from "react";
import ReactDOM from "react-dom";
import FileForm from "../FileForm";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<FileForm />, div);
});
