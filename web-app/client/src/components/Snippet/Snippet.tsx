/* eslint-disable no-console */
import React, { useState, useEffect } from "react";
import CSS from "csstype";
import { dependency } from "../../types";
import "./Snippet.css";

interface Props {
  file: File | null;
  selectedDependency: dependency | undefined;
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
  arrMatches = objPattern.exec(inputData);
  while (arrMatches !== null) {
    const strMatchedDelimiter = arrMatches[1];
    if (strMatchedDelimiter.length && strMatchedDelimiter !== delimiter) {
      arrData.push([]);
    }

    let strMatchedValue;

    if (arrMatches[2]) {
      strMatchedValue = arrMatches[2].replace(new RegExp("\"\"", "g"), "\"");
    } else {
      strMatchedValue = arrMatches[3];
    }

    arrData[arrData.length - 1].push(strMatchedValue);
    arrMatches = objPattern.exec(inputData);
  }

  return arrData;
}

function getSelectedIndices(dep: dependency | undefined, header: string[]): number[] {
  const selectedIndices: number[] = [];
  if (dep === undefined) {
    return selectedIndices;
  }
  dep.lhs.forEach((lhs) => selectedIndices.push(header.indexOf(lhs.name)));
  selectedIndices.push(header.indexOf(dep.rhs.name));
  return selectedIndices;
}

const Snippet: React.FC<Props> = ({ file, selectedDependency }) => {
  const [table, setTable] = useState<string[][]>([[]]);
  const selectedStyle: CSS.Properties = {
    backgroundColor: "#8792C0",
  };
  const defaultStyle: CSS.Properties = {};
  const selectedIndices: number[] = getSelectedIndices(selectedDependency, table[0]);
  console.log(selectedIndices);

  useEffect(() => {
    function readFile() {
      file!.text().then((buffer) => setTable(convertCSVToArray(buffer, ",")));
    }
    readFile();
  }, []);

  return (
    <table>
      {
        table === undefined ?
          <></>
          :
          table!.map((value, index) => (
            <tr
              // eslint-disable-next-line react/no-array-index-key
              key={index}
            >
              {value!.map((cell, idx) => (
                <td
                  // eslint-disable-next-line react/no-array-index-key
                  key={idx}
                  style={selectedIndices.indexOf(idx) !== -1 ? selectedStyle : defaultStyle}
                >
                  {cell}
                </td>
              ))}
            </tr>

          ))
      }
    </table>
  );
};

export default Snippet;
