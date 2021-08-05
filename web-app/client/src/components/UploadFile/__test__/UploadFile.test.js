import React from "react";
import ReactDOM from "react-dom";
import UploadFile from "../UploadFile";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(<UploadFile />, div);
});

