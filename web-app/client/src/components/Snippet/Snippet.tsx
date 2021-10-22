// import React, { useState, useEffect } from "react";
/* eslint-disable no-console */
import React, { useState, useEffect } from "react";

interface Props {
  file: File | null;
}

/* eslint-disable prefer-template */
/* eslint-disable no-cond-assign */
/* eslint-disable prefer-destructuring */

function convertCSVToArray(inputData: string, delimiter: string) {
  const objPattern = new RegExp(
    "(\\" +
      delimiter +
      "|\\r?\\n|\\r|^)" +
      "(?:\"([^\"]*(?:\"\"[^\"]*)*)\"|" +
      "([^\"\\" +
      delimiter +
      "\\r\\n]*))",
    "gi",
  );

  let arrMatches: any;
  const arrData: string[][] = [[]];

  while ((arrMatches = objPattern.exec(inputData))) {
    const strMatchedDelimiter = arrMatches[1];
    if (
      strMatchedDelimiter.length &&
      strMatchedDelimiter !== delimiter
    ) {
      arrData.push([]);
    }

    let strMatchedValue;

    if (arrMatches[2]) {
      strMatchedValue = arrMatches[2].replace(new RegExp("\"\"", "g"), "\"");
    } else {
      strMatchedValue = arrMatches[3];
    }

    arrData[arrData.length - 1].push(strMatchedValue);
  }

  return arrData;
}

const Snippet: React.FC<Props> = ({ file }) => {
  const [text, setText] = useState("");

  const [table, setTable] = useState<string[][]>();

  useEffect(() => {
    async function readFile() {
      const buffer = await file!.text();
      setText(buffer);
      setTable(convertCSVToArray(text, ","));
      console.table(table);
    }
    readFile();
  }, []);

  return <div>{table}</div>;
};

export default Snippet;
