import React, { useEffect, useState } from "react";
import { Loader, Switcher } from "@skbkontur/react-ui";
// eslint-disable-next-line no-unused-vars
import { CreateTaskButtons } from "./CreateButtons";
import "./TestPage.css";
import { CreateTask } from "./CreateTask";

export const TestPage = () => (
  <div className="App-test">
    <header className="App-header-test">
      <div>
        <AlgList />
        {/* <CreateTaskButtons /> */}
        <CreateTask />
      </div>
    </header>
  </div>
);

const AlgList = () => {
  const [error, setError] = useState<{message: string} | null>(null);
  const [isLoaded, setIsLoaded] = useState<boolean>(false);
  const [algNames, setAlgNames] = useState<string[]|null>(null);
  const [value, setValue] = useState<string | null>(null);

  useEffect(() => {
    fetch("http://localhost:5000/algsInfo")
      .then((res) => res.json())
      .then(
        (result) => {
          setIsLoaded(true);
          const res: string[] = [];
          result.allowedAlgorithms.map((name: string) => res.push(name));
          setAlgNames(res);
        },
        (err) => {
          setError(err);
          setIsLoaded(true);
        },
      );
  }, []);

  if (error) {
    return (
      <div>
        Ошибка:
        {" "}
        {error.message}
      </div>
    );
  }
  return (
    <div>
      <Loader active={!isLoaded}>
        {
            algNames
            && (
            <Switcher
              label="Выберите алгоритм"
              items={algNames}
              value={value || undefined}
              onValueChange={setValue}
            />
            )
        }
      </Loader>
    </div>
  );
};
