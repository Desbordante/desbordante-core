import React from "react";
import ReactDOM from "react-dom";
import FileLabel from "../FileLabel";

import "../../../mocks";

it("renders without crashing", () => {
  const div = document.createElement("div");
  ReactDOM.render(
    <FileLabel
      fileExistenceValidatorFunc={() => true}
      fileSizeValidatorFunc={() => true}
      fileFormatValidatorFunc={() => true}
      file={{ name: "filename" }}
    />,
    div
  );
});
