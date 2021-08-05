/* eslint-disable */

import "./App.css";
import React, { useState, useRef, useEffect } from "react";
import axios from "axios";
import { disableBodyScroll } from "body-scroll-lock";
import Header from "./components/Header/Header";
import LoadingScreen from "./components/LoadingScreen/LoadingScreen";
import FileForm from "./components/FileForm/FileForm";
import Viewer from "./components/Viewer/Viewer";
import ProgressBar from "./components/ProgressBar/ProgressBar";
import { getData } from "./APIFunctions";
import Button from "./components/Button/Button";

function App() {
  disableBodyScroll(document);

  // State describes what screen should be seen at the moment
  const [state, setState] = useState(0);

  const [filename, setFilename] = useState("");
  const [uploadProgress, setUploadProgress] = useState(0.0);

  const [taskID, setTaskID] = useState("");
  const [taskStatus, setTaskStatus] = useState("UNSCHEDULED");
  const [taskProgress, setTaskProgress] = useState(0);

  const [dependencies, setDependencies] = useState([]);
  const [attributes, setAttributes] = useState({ lhs: [], rhs: [] });

  const reset = () => {
    setState(0);
    setUploadProgress(0);
    setTaskID("");
    setTaskStatus("UNSCHEDULED");
    setTaskProgress(0);
    setDependencies([]);
    setAttributes({ lhs: [], rhs: [] });
  }

  const taskFinished = (status) => ["COMPLETED", "ERROR"].includes(status);

  useEffect(() => {
    // console.log(state);
    // console.log("taskID!!", taskID);
    // console.log("taskStatus!!", taskStatus);
    // console.log("Deps!!", dependencies);
    // console.log("Attrs!!", attributes);
    // console.log("===========================");
  });

  useEffect(() => {
    const fetchData = async () => {
      // console.log("UPDATE");
      const task = await getData(`getTaskInfo?taskID=${taskID}`);
      // console.log(task);
      setTaskStatus(task.status);
      setTaskProgress(task.progress / 100);

      if (taskFinished(task.status)) {
        setAttributes({
          lhs: task.jsonarraynamevalue.lhs.filter((attr) => attr.value > 0),
          rhs: task.jsonarraynamevalue.rhs.filter((attr) => attr.value > 0),
        });
        setDependencies(
          task.fds
            .filter((dep) => dep.lhs.length > 0)
            .map((dep) => ({
              lhs: dep.lhs.map(
                (attr) => task.jsonarraynamevalue.lhs[attr].name
              ),
              rhs: task.jsonarraynamevalue.lhs[dep.rhs].name,
            }))
        );
      }
    };

    const timer = setInterval(() => {
      if (taskID !== "" && !taskFinished(taskStatus)) {
        fetchData();
      }
    }, 2000);

    return () => clearInterval(timer);
  });

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
    // console.log(state);
  }, [state]);

  useEffect(
    () =>
      fileScreen.current.scrollIntoView({
        behavior: "smooth",
      }),
    []
  );

  return (
    <div className="App">
      <div className="screen" ref={fileScreen}>
        <Header />
        <FileForm
          onSubmit={() => setState(1)}
          onUploadProgress={setUploadProgress}
          cancelTokenSource={cancelTokenSource}
          setFilename={setFilename}
          setTaskID={setTaskID}
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
            <div className="left">
              <img src="/icons/logo.svg" alt="logo" className="logo-medium" />
              <h1>File: "{filename}"</h1>
              <h1>Status: {taskStatus}</h1>
            </div>
            <Button text="Cancel" style={{ backgroundColor: "var(--red)" }} onClick={reset}/>
          </header>
          <ProgressBar
            maxWidth={100}
            widthUnit="%"
            progress={taskProgress}
            thickness={0.5}
            rounded={false}
            transition={0.1}
          />
        </div>
        <Viewer
          currentState={1}
          attributesLHS={attributes.lhs}
          attributesRHS={attributes.rhs}
          dependencies={dependencies}
          taskFinished={taskFinished(taskStatus)}
          taskStatus={taskStatus}
        />
      </div>
    </div>
  );
}

export default App;
