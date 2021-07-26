import React from "react";
import { BrowserRouter as Router, Switch, Route } from "react-router-dom";
import { TestPage } from "./TestPage";
import { FileUpload } from "./Upload";

export const App = () => (
  <Router>
    <Switch>
      <Route path="/test">
        <TestPage />
      </Route>
      <Route path="/">
        <div>
          <FileUpload />
        </div>
      </Route>
    </Switch>
  </Router>
);
