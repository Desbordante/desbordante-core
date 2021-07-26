import "./index.css";
import React from "react";
import ReactDOM from "react-dom";
import { BrowserRouter as Router, Route, Switch } from "react-router-dom";
import App from "./App";
import { App2 } from "./experiments/App2";

ReactDOM.render(
  <Router>
    <Switch>
      <Route path="/experiments">
        <App2 />
      </Route>
      <Route path="/">
        <App />
      </Route>
    </Switch>
  </Router>,
  document.getElementById("root"),
);
