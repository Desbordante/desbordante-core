/* eslint-disable */

import React, { useState, useEffect, useRef } from "react";
import {
  BrowserRouter as Router,
  Route,
  Switch,
  Link,
  useParams,
  useHistory,
} from "react-router-dom";
import axios from "axios";

import "./Viewer.css";
import PieChartFull from "../PieChartFull/PieChartFull";
import DependencyListFull from "../DependencyListFull/DependencyListFull";
import StatusDisplay from "../StatusDisplay/StatusDisplay";
import Button from "../Button/Button";
import ProgressBar from "../ProgressBar/ProgressBar";
import { serverURL } from "../../APIFunctions";
import {
  taskStatus,
  attribute,
  dependency,
  dependencyEncoded,
  coloredAttribute,
  coloredDepedency,
} from "../../types";

interface Props {
  file: File | null;
  setFile: (file: File | null) => void;
}

const Viewer: React.FC<Props> = ({ file, setFile }) => {
  let { taskID } = useParams<{ taskID: string }>();
  const history = useHistory();

  const [taskProgress, setTaskProgress] = useState(0);
  const [taskStatus, setTaskStatus] = useState<taskStatus>("UNSCHEDULED");
  const [filename, setFilename] = useState<string | null>("Todo"); //TODO: add file name to taskInfo

  const [attributesLHS, setAttributesLHS] = useState<coloredAttribute[]>([]);
  const [attributesRHS, setAttributesRHS] = useState<coloredAttribute[]>([]);

  const [selectedAttributesLHS, setSelectedAttributesLHS] = useState<
    coloredAttribute[]
  >([]);
  const [selectedAttributesRHS, setSelectedAttributesRHS] = useState<
    coloredAttribute[]
  >([]);

  const [dependencies, setDependencies] = useState<dependency[]>([]);

  const dependencyColors: string[] = [
    "#ff5757",
    "#575fff",
    "#4de3a2",
    "#edc645",
    "#d159de",
    "#32bbc2",
    "#ffa857",
    "#8dd44a",
    "#6298d1",
    "#969696",
  ]

  function createColoredDep(dep: dependency, colorsBuffer: string[]): coloredDepedency {
    return {
      lhs: dep.lhs.map((attr) => ({
        name: attr.name,
        value: attr.value,
        color: pickRandomColor(colorsBuffer)
      })),
      rhs: {
        name: dep.rhs.name,
        value: dep.rhs.value,
        color: pickRandomColor(colorsBuffer)
      }
    }
  }

  const pickRandomColor = (colors: string[]) => {
    const pickedIndex = Math.floor(Math.random() * colors.length);
    const pickedElement = colors[pickedIndex];
    colors.splice(pickedIndex, 1)
    return pickedElement;
  }

  const taskFinished = (status: taskStatus) =>
    status === "COMPLETED" || status === "SERVER ERROR";

  useEffect(() => {
    const fetchData = async () => {
      axios
        .get(`${serverURL}/getTaskInfo?taskID=${taskID}`, { timeout: 2000 })
        .then((task) => task.data)
        .then((data) => {
          console.log(data);
          setTaskProgress(data.progress);
          setTaskStatus(data.status);
          if (taskFinished(data.status)) {
            setAttributesLHS(data.arraynamevalue.lhs);
            setAttributesRHS(data.arraynamevalue.rhs);
            setDependencies(
              data.fds.map((dep: dependencyEncoded) => ({
                lhs: dep.lhs.map(
                  (attrNum) =>
                    data.arraynamevalue.lhs[attrNum] ||
                    data.arraynamevalue.lhs[0]
                ),
                rhs:
                  data.arraynamevalue.rhs[dep.rhs] ||
                  data.arraynamevalue.rhs[0],
              }))
            );
          }
        })
        .catch((error) => {
          console.error(error);
          history.push("/error");
        });
    };

    const timer = setInterval(() => {
      if (!taskFinished(taskStatus)) {
        fetchData();
      }
    }, 1000);

    return () => clearInterval(timer);
  });

  return (
    <>
      <div className="top-bar">
        <header>
          <div className="left">
            <img src="/icons/logo.svg" alt="logo" className="logo-medium" />
            <h1>File: "{filename}"</h1>
            <h1>Status: {taskStatus}</h1>
          </div>
          <Button type="button" onClick={() => {
            history.push("/")
            setFile(null)
          }}>
            Cancel
          </Button>
        </header>
        <ProgressBar
          progress={taskProgress / 100}
          maxWidth={100}
          thickness={0.5}
        />
      </div>
      <Router>
        {/* <Switch> */}
        <Route path={`/attrs/${taskID}`}>
          <div className="bg-light">
            {taskFinished(taskStatus) ? null : (
              <StatusDisplay type="loading" text="Loading" />
            )}
            <div
              className="charts-with-controls"
              style={{
                opacity: taskFinished(taskStatus) ? 1 : 0,
              }}
            >
              <PieChartFull
                title="Left-hand side"
                attributes={attributesLHS}
                selectedAttributes={selectedAttributesLHS}
                setSelectedAttributes={setSelectedAttributesLHS}
              />
              <PieChartFull
                title="Right-hand side"
                attributes={attributesRHS}
                maxItemsSelected={1}
                selectedAttributes={selectedAttributesRHS}
                setSelectedAttributes={setSelectedAttributesRHS}
              />
            </div>
            <footer style={{ opacity: taskFinished(taskStatus) ? 1 : 0 }}>
              <h1
                className="bottom-title"
                style={{ color: "#000000", fontWeight: 500 }}
              >
                View Dependencies
              </h1>
              <Link to={`/deps/${taskID}`}>
                <Button
                  type="button"
                  color="gradient"
                  glow="always"
                  onClick={() => { }}
                >
                  <img src="/icons/nav-down.svg" alt="down" />
                </Button>
              </Link>
            </footer>
          </div>
        </Route>
        <Route path={`/deps/${taskID}`}>
          <div className="bg-light" style={{ justifyContent: "space-between" }}>
            <DependencyListFull
              file={file}
              dependencies={dependencies.map((dep) => {
                return createColoredDep(dep, dependencyColors.slice(0))
              })}
              selectedAttributesLHS={selectedAttributesLHS}
              selectedAttributesRHS={selectedAttributesRHS}
            />
            <footer>
              <h1
                className="bottom-title"
                style={{ color: "#000", fontWeight: 500 }}
              >
                View Attributes
              </h1>
              <Link to={`/attrs/${taskID}`}>
                <Button
                  type="button"
                  color="gradient"
                  glow="always"
                  onClick={() => { }}
                >
                  <img src="/icons/nav-up.svg" alt="up" />
                </Button>
              </Link>
            </footer>
          </div>
        </Route>
        {/* </Switch> */}
      </Router>
    </>
  );
};

export default Viewer;
