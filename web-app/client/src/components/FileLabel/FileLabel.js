/* eslint-disable */
import React, { useState } from "react";
import "./FileLabel.css";
import { useDropzone } from "react-dropzone";

function FileLabel({
  file,
  fileExistenceValidatorFunc,
  fileSizeValidatorFunc,
  fileFormatValidatorFunc,
  onClick,
  setFile,
}) {
  // Is the input field glowing
  const [glow, setGlow] = useState(false);

  // Needed for dropzone
  const { getRootProps, getInputProps } = useDropzone({
    onDrop: (acceptedFiles) => setFile(acceptedFiles[0]),
  });

  // Make borders red if error occurs
  const borderClass = fileExistenceValidatorFunc(file)
    ? fileSizeValidatorFunc(file) && fileFormatValidatorFunc(file)
      ? "active"
      : "error"
    : "inactive";

  // show error if can't process file
  let fileTitle = (
    <>
      <span className="hilight-green">Upload</span> your dataset, or
      <span className="hilight-purple"> choose</span> one of ours ...
    </>
  );
  if (fileExistenceValidatorFunc(file)) {
    if (fileSizeValidatorFunc(file) && fileFormatValidatorFunc(file)) {
      fileTitle = file.name;
    } else {
      if (fileSizeValidatorFunc(file)) {
        fileTitle = (
          <span className="hilight-red">
            Error: this format is not supported
          </span>
        );
      } else {
        fileTitle = (
          <span className="hilight-red">Error: this file is too large</span>
        );
      }
    }
  }

  return (
    <div
      className="file-name-wrapper"
      // style={{
      //   position: "relative",
      //   transform: glow ? "scale(1.03)" : "scale(1)",
      // }}
      {...getRootProps()}
    >
      <div
        className={`round-corners gradient-fill ${borderClass} glow`}
        style={{
          zIndex: 0,
          width: "30.5rem",
          height: "3.2rem",
          filter: "blur(1rem)",
          opacity: glow ? 1 : 0,
        }}
        onClick={onClick}
      ></div>
      <div
        className={`round-corners gradient-fill wrapper ${borderClass}`}
        onClick={onClick}
        onMouseEnter={() => setGlow(true)}
        onMouseLeave={() => setGlow(false)}
      >
        <div className="round-corners filename">{fileTitle}</div>
      </div>
    </div>
  );
}

export default FileLabel;
