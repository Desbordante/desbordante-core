/* eslint-disable no-console */

import "./App.css";
import React, { useState } from "react";
import { Switch, BrowserRouter as Router, Route } from "react-router-dom";

import LoadingScreen from "./components/LoadingScreen/LoadingScreen";
import HomeScreen from "./components/HomeScreen/HomeScreen";
import ErrorScreen from "./components/ErrorScreen/ErrorScreen";
import Viewer from "./components/Viewer/Viewer";

const App: React.FC = () => {
  const [uploadProgress, setUploadProgress] = useState(0.0);
  const [file, setFile] = useState<File | null>(null);

  return (
    <Router>
      <Switch>
        <div className="App">
          {/* Error Page */}
          <Route path="/error" exact>
            <ErrorScreen code="404" message="can't connect to the server." />
          </Route>

          {/* Loading Page */}
          <Route path="/loading" exact>
            <LoadingScreen
              onComplete={() => {}} // TODO:
              progress={uploadProgress}
            />
          </Route>

          {/* View Page */}
          <Route path="/(attrs|deps)/:taskID/">
            <div className="screen">
              <Viewer file={file} />
            </div>
          </Route>

          {/* Home Page */}
          <Route path="/" exact>
            <HomeScreen
              file={file}
              setFile={setFile}
              setUploadProgress={setUploadProgress}
            />
          </Route>
        </div>
      </Switch>
    </Router>
  );
};

export default App;
