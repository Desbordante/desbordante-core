import axios from "axios";
import { Button } from "@skbkontur/react-ui";
import React from "react";

export const ChooseTask = () => {
  function onFileUpload() {
    const json = JSON.stringify({
      algName: "Tane", semicolon: ",", errorPercent: 0.02, fileName: "TestLong.csv",
    });
    // eslint-disable-next-line no-console
    console.log(json);

    const blob = new Blob([json], {
      type: "application/json",
    });
    const data = new FormData();
    data.append("document", blob);

    axios({
      method: "post",
      url: "http://localhost:5000/chooseTask",
      data,
    });
  }

  return (
    <div style={{ width: "400px" }}>
      <h3>
        Create task (with choosing available dataset)
      </h3>
      <div>
        <Button type="button" onClick={onFileUpload}>
          Create new task
        </Button>
        <div>
          You can see hardcoded JSON in Console (after click button)
        </div>
      </div>
    </div>
  );
};
