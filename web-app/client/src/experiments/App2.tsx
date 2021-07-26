import React from "react";
import { Switch, Route } from "react-router-dom";
import { TestPage } from "./TestPage";
import { CreateTask } from "./CreateTask";

export const App2 = () => (
  <Switch>
    <Route path="/experiments/upload">
      <CreateTask />
    </Route>
    <Route path="/experiments">
      <div>
        <TestPage />
      </div>
    </Route>
  </Switch>
);
