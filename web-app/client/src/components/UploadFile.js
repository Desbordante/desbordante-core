import React, { useRef } from "react";
import "./UploadFile.css";
import FileLabel from "./FileLabel";
import ButtonIcon from "./ButtonIcon";

function UploadFile({
  onClick,
  file,
  fileExistenceValidatorFunc,
  fileSizeValidatorFunc,
  fileFormatValidatorFunc,
}) {
  const inputFile = useRef(null);

  const onButtonClick = () => {
    inputFile.current.click();
  };

  return (
    <>
      <FileLabel
        file={file}
        fileExistenceValidatorFunc={fileExistenceValidatorFunc}
        fileSizeValidatorFunc={fileSizeValidatorFunc}
        fileFormatValidatorFunc={fileFormatValidatorFunc}
      />
      <input
        type="file"
        ref={inputFile}
        id="file"
        onChange={(e) => onClick(e.target.files[0])}
        multiple={false}
        accept=".csv, .CSV"
      />
      <ButtonIcon
        src="/icons/upload.svg"
        alt="Upload"
        onClick={onButtonClick}
        color="green"
        glow
      />
    </>
  );
}

export default UploadFile;
