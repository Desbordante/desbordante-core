import React, { useRef } from "react";
import "./UploadFile.css";
import FileLabel from "../FileLabel/FileLabel";
import Button from "../Button/Button";

/* eslint-disable no-unused-vars */
interface Props {
  file: File | null;
  onClick: (file: File) => void;
  fileExistenceValidator: (file: File | null) => boolean;
  fileSizeValidator: (file: File | null) => boolean;
  fileFormatValidator: (file: File | null) => boolean;
  openPopupWindow: () => void;
  builtinDataset: string | null;
  disableBuiltinDataset: () => void;
}
/* eslint-enable no-unused-vars */

const UploadFile: React.FC<Props> = ({
  file,
  onClick,
  fileExistenceValidator,
  fileSizeValidator,
  fileFormatValidator,
  openPopupWindow,
  builtinDataset,
  disableBuiltinDataset,
}) => {
  // you can only use <input type="file" /> for choosing files,
  // so the reference is used to forward click action
  // from regular button to hidden input file

  const inputFile = useRef<HTMLInputElement>(null);

  return (
    <>
      <FileLabel
        file={file}
        builtinDataset={builtinDataset}
        onClick={() => inputFile?.current?.click()}
        fileExistenceValidator={fileExistenceValidator}
        fileSizeValidator={fileSizeValidator}
        fileFormatValidator={fileFormatValidator}
        setFile={onClick}
      />
      <input
        type="file"
        id="file"
        ref={inputFile}
        onChange={(e) => {
          /* eslint no-console: 1 */
          console.log(e.target.files);
          if (e.target.files) {
            onClick(e.target.files[0]);
          }
          disableBuiltinDataset();
        }}
        multiple={false}
        accept=".csv, .CSV"
      />
      <Button
        onClick={() => inputFile?.current?.click()}
        color="0"
        type="button"
      >
        <img src="/icons/upload.svg" alt="upload" className="upload-icon" />
      </Button>
      <Button onClick={openPopupWindow} type="button" color="1">
        <img src="/icons/choose.svg" alt="choose" className="upload-icon" />
      </Button>
    </>
  );
};

export default UploadFile;
