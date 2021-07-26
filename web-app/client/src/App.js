import "./App.css";
import React, { useState, useRef } from "react";
// eslint-disable-next-line no-unused-vars
import { disableBodyScroll, enableBodyScroll } from "body-scroll-lock";
import Header from "./components/Header";
import LoadingScreen from "./components/LoadingScreen";
import FileForm from "./components/FileForm";
import Viewer from "./components/Viewer";
import ProgressBar from "./components/ProgressBar";

function App() {
  // disableBodyScroll(document);
  const [state, setState] = useState(0);
  const loadingScreen = useRef();
  const viewerScreen = useRef();

  return (
    <div className="App">
      <div className="screen">
        <Header />
        <FileForm
          onSubmit={() => {
            setState(1);
            loadingScreen.current.scrollIntoView({ behavior: "smooth" });
          }}
        />
      </div>
      <div className="screen" ref={loadingScreen}>
        <LoadingScreen
          onComplete={() => {
            setState(2);
            viewerScreen.current.scrollIntoView({ behavior: "smooth" });
          }}
          state={state}
        />
      </div>
      <div className="screen" ref={viewerScreen}>
        <header>
          <img src="/icons/logo.svg" alt="logo" className="logo-medium" />
        </header>
        <ProgressBar
          maxWidth={100}
          widthUnit="%"
          percent={0.6}
          thickness={0.5}
          rounded={false}
        />
        <Viewer />
      </div>
    </div>
  );
}

export default App;
