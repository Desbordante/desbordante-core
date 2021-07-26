import React, { useState } from "react";
import { Button } from "@skbkontur/react-ui";

export const CreateTaskButtons = () => {
  const [status, setStatus] = useState<string | undefined>(undefined);
  const [taskID, setTaskID] = useState<string | undefined>(undefined);

  function createTask() {
    const requestOptions = {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ algName: "Tane", semicolon: ",", errorPercent: 0.02 }),
    };
    fetch("http://localhost:5000/createTask", requestOptions)
      .then((res) => res.json())
      .then((data) => {
        setStatus(data.status);
        setTaskID(data.taskID);
      });
  }

  return (
    <div>
      <Button onClick={createTask}>Create new task</Button>
      <div style={{
        margin: "15px",
        padding: "10px",
        width: " 400px",
        display: "flex",
        flexDirection: "column",
        color: "antiquewhite",
        backgroundColor: "#3b3737",
        borderRadius: "10px",
        border: "aliceblue solid 2px",
        alignItems: "flex-start",
      }}
      >
        <span style={{ color: "white" }}>Answer:</span>
        <div>
          Status --
          {" "}
          {status || "Undefined"}
        </div>
        <div style={{ textAlign: "left" }}>
          Task ID --
          {" "}
          {taskID || "Undefined"}
        </div>
      </div>
    </div>
  );
};
