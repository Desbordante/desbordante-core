/* eslint-disable */
import React from "react";
import "./FileLabel.css";

function FileLabel({ file, fileExistenceValidatorFunc, fileSizeValidatorFunc, fileFormatValidatorFunc }) {
  // Make borders red if error occurs
  const borderClass = fileExistenceValidatorFunc(file) ?
    (fileSizeValidatorFunc(file) && fileFormatValidatorFunc(file)
      ? "active"
      : "error") : "inactive";

  // show error if can't process file
  let fileTitle = <><span className="hilight-purple">Upload</span>
      {" "}
      your dataset, or
      <span className="hilight-green"> choose</span>
      {" "}
      one of ours ...</>;
  if (fileExistenceValidatorFunc(file)) {
    if (fileSizeValidatorFunc(file) && fileFormatValidatorFunc(file)) {
      fileTitle = file.name;
    } else {
      if (fileSizeValidatorFunc(file)) {
        fileTitle = <span className="hilight-red">Error: this format is not supported</span>;
      } else {
        fileTitle = <span className="hilight-red">Error: this file is too large</span>;
      }
    }
  }
  
  return (
    <>
      <div className={`round-corners gradient-fill wrapper ${borderClass}`}>
        <div className="round-corners filename">{fileTitle}</div>
      </div>
    </>
  );
}

export default FileLabel;
