import axios from "axios";
import React, { useState } from "react";
import { Button } from "@skbkontur/react-ui";

export const CreateTask = () => {
  const [selectedFile, setSelectedFile] = useState<File>();

  function onFileChange(event:React.ChangeEvent<HTMLInputElement>) {
    setSelectedFile(event.target.files![0]);
  }

  function onFileUpload() {
    const json = JSON.stringify({ algName: "Tane", semicolon: ",", errorPercent: 0.02 });
    const blob = new Blob([json], {
      type: "application/json",
    });
    // Create an object of formData
    const data = new FormData();

    // Update the formData object
    if (selectedFile) {
      data.append(
        "file",
        selectedFile,
        selectedFile.name,
      );
    }

    data.append("document", blob);

    axios({
      method: "post",
      url: "http://localhost:5000/createTask",
      data,
      headers: { "Content-Type": "text/csv" },
    });
  }

  function fileData() {
    if (selectedFile) {
      return (
        <div>
          <h2>File Details:</h2>
          <p>
            File Name:
            {selectedFile.name}
          </p>
          <p>
            File Type:
            {selectedFile.type}
          </p>
        </div>
      );
    }
    return (
      <div>
        <br />
        <h4>Choose before Pressing the Create button</h4>
      </div>
    );
  }

  return (
    <div style={{ width: "400px" }}>
      <h3>
        Create task (with file uploading)!
      </h3>
      <div>
        <input type="file" onChange={onFileChange} />
        <Button type="button" onClick={onFileUpload}>
          Create new task
        </Button>
      </div>
      {fileData()}
    </div>
  );
};
