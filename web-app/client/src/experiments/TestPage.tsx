import React from "react";
import "./TestPage.css";
import { CreateTask } from "./CreateTask";
import { ChooseTask } from "./ChooseTask";

export const TestPage = () => (
  <div className="App-test">
    <header className="App-header-test">
      <div>
        <CreateTask />
        <ChooseTask />
      </div>
    </header>
  </div>
);
