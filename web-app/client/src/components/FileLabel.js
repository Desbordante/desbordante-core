import React from "react";
import "./FileLabel.css";

function FileLabel({ file, validatorFunc }) {
  const borderClass =
    file === null
      ? "inactive"
      : validatorFunc(file)
      ? "active"
      : "error";

  const fileTitle =
    file === null ? (
      <>
        <span className="hilight-purple">Upload</span> your dataset, or
        <span className="hilight-green"> choose</span> one of ours ...
      </>
    ) : validatorFunc(file) ? (
      <>{file.name}</>
    ) : (
      <span className="hilight-red">Error: file is too large!</span>
    );

  return (
    <>
      <div className={`round-corners gradient-fill wrapper ${borderClass}`}>
        <div className="round-corners filename">{fileTitle}</div>
      </div>
    </>
  );
}

export default FileLabel;
