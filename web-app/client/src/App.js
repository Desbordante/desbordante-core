import "./App.css";
import React, { useState, useRef, useEffect } from "react";
import axios from "axios";
import { disableBodyScroll } from "body-scroll-lock";
import Header from "./components/Header";
import LoadingScreen from "./components/LoadingScreen";
import FileForm from "./components/FileForm";
import Viewer from "./components/Viewer";
import ProgressBar from "./components/ProgressBar";

function App() {
  disableBodyScroll(document);

  // State describes what screen should be seen at the moment
  const [state, setState] = useState(0);

  const [filename, setFilename] = useState("");

  const [uploadProgress, setUploadProgress] = useState(0.0);

  // Refs to screens
  const fileScreen = useRef();
  const loadingScreen = useRef();
  const viewerScreen = useRef();

  // Cancel token for file upload
  const cancelTokenSource = axios.CancelToken.source();

  // Scroll to screen according to state
  useEffect(() => {
    [fileScreen, loadingScreen, viewerScreen][state].current.scrollIntoView({
      behavior: "smooth",
    });
  }, [state]);

  return (
    <div className="App">
      <div className="screen" ref={fileScreen}>
        <Header />
        <FileForm
          onSubmit={() => setState(1)}
          onUploadProgress={setUploadProgress}
          cancelTokenSource={cancelTokenSource}
          setFilename={setFilename}
        />
      </div>
      <div className="screen" ref={loadingScreen}>
        <LoadingScreen
          onComplete={() => setState(2)}
          progress={uploadProgress}
          onCancel={() => {
            cancelTokenSource.cancel("Upload cancelled");
            setUploadProgress(0.0);
            setState(0);
          }}
        />
      </div>
      <div
        className="screen"
        ref={viewerScreen}
        style={{ height: "calc(200vh - 6.5rem)" }}
      >
        <div className="top-bar">
          <header>
            <img src="/icons/logo.svg" alt="logo" className="logo-medium" />
            {filename}
          </header>
          <ProgressBar
            maxWidth={100}
            widthUnit="%"
            progress={0.6}
            thickness={0.5}
            rounded={false}
          />
        </div>
        <Viewer currentState={1} />
      </div>
    </div>
  );
}

export default App;
