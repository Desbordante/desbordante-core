import React, { useEffect, useState } from "react";
import "./App.css";
import { Switcher, Loader } from "@skbkontur/react-ui";

const App = () => {
  const [error, setError] = useState<{message: string} | null>(null);
  const [isLoaded, setIsLoaded] = useState<boolean>(false);
  const [algNames, setAlgNames] = useState<string[]|null>(null);
  const [value, setValue] = useState<string | null>(null);

  useEffect(() => {
    fetch("http://localhost:5000/algorithms")
      .then((res) => res.json())
      .then(
        (result) => {
          setIsLoaded(true);
          const res: string[] = [];
          result.algorithms.map((item:{ name : string }) => res.push(item.name));
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
        {error.message}
      </div>
    );
  }
  return (
    <div className="App">
      <header className="App-header">
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
      </header>
    </div>
  );
};

export default App;
