import "./index.css";
import React from "react";
import ReactDOM from "react-dom";
import { BrowserRouter as Router, Route, Switch } from "react-router-dom";
import App from "./App";
import { TestPage } from "./experiments/TestPage";

ReactDOM.render(
  <Router>
    <Switch>
      <Route path="/experiments">
        <TestPage />
      </Route>
      <Route path="/">
        <App />
      </Route>
    </Switch>
  </Router>,
  document.getElementById("root"),
);
