import React, { useState, useEffect } from "react";
import "./FileForm.css";
import Checkbox from "./Checkbox";
import Submit from "./Submit";
import Radio from "./Radio";
import Value from "./Value";
import Slider from "./Slider";
import UploadFile from "./UploadFile";
import { serverURL, sendData } from "../APIFunctions";

function FileForm({ onSubmit }) {
  // Allowed field values
  const [allowedSeparators, setAllowedSeparators] = useState([
    ",",
    "\\t",
    "\\n",
  ]);
  const [allowedAlgorithms, setAllowedAlgorithms] = useState([]);
  const [maxfilesize, setMaxFileSize] = useState(5e7);

  // Parameters, later sent to the server on execution as JSON
  const [file, setFile] = useState(null);
  const [hasHeader, setHasHeader] = useState(true);
  const [separator, setSeparator] = useState(allowedSeparators[0]);
  const [algorithm, setAlgorithm] = useState("");
  const [errorThreshold, setErrorThreshold] = useState(0.05);
  const [maxLHSAttributes, setMaxLHSAttributes] = useState(5);

  useEffect(() => {
    fetch(`${serverURL}/algorithms`)
      .then((res) => res.json())
      .then((res) => res.algorithms.map(({ name }) => name))
      .then((res) => {
        setAllowedAlgorithms(res);
        setAlgorithm(res[0]);
      });
  }, []);

  // useEffect(() => setAlgorithm(allowedAlgorithms[0]))

  // Validator functions for fields
  const fileExistenceValidatorFunc = (file) => !!file;
  const fileSizeValidatorFunc = (file) =>
    file == null || file.size < maxfilesize;
  const separatorValidatorFunc = (n) => allowedSeparators.indexOf(n) !== -1;
  const errorValidatorFunc = (n) => !isNaN(n) && n >= 0 && n <= 1;
  const maxLHSValidatorFunc = (n) => !isNaN(n) && n > 0 && n % 1 === 0;

  // Validator function that ensures every field is correct
  function isValid(options) {
    return (
      fileExistenceValidatorFunc(file) &&
      fileSizeValidatorFunc(file) &&
      separatorValidatorFunc(separator) &&
      errorValidatorFunc(errorThreshold) &&
      maxLHSValidatorFunc(maxLHSAttributes)
    );
  }
  console.log(algorithm);

  return (
    <form>
      <div className="form-item">
        <h3>1.</h3>
        <UploadFile
          onClick={setFile}
          file={file}
          validatorFunc={fileSizeValidatorFunc}
          color="purple"
        />
      </div>
      <div className="form-item">
        <h3>2. File properties</h3>
        <Checkbox text="Header" toggleObj={hasHeader} onClick={setHasHeader} />
        <h3>separator</h3>
        <Value
          toggleObj={separator}
          onChange={setSeparator}
          size={1}
          validatorFunc={separatorValidatorFunc}
        />
      </div>
      <div className="form-item">
        <h3>3. Algorithm</h3>
        {allowedAlgorithms.map((algo) => (
          <Radio
            text={algo}
            onClick={setAlgorithm}
            toggleObj={algorithm}
            key={algo}
          />
        ))}
      </div>
      <div className="form-item">
        <h3>4. Error threshold</h3>
        <Value
          toggleObj={errorThreshold}
          onChange={setErrorThreshold}
          size={3}
          validatorFunc={errorValidatorFunc}
        />
        <Slider
          toggleObj={errorThreshold}
          onChange={setErrorThreshold}
          min={0}
          max={1}
          step={0.001}
          exponential={true}
          validatorFunc={errorValidatorFunc}
        />
      </div>
      <div className="form-item">
        <h3>5. Max LHS attributes</h3>
        <Value
          toggleObj={maxLHSAttributes}
          onChange={setMaxLHSAttributes}
          min={1}
          max={100}
          integer={true}
          size={3}
          validatorFunc={maxLHSValidatorFunc}
        />
        <Slider
          min={1}
          max={10}
          toggleObj={maxLHSAttributes}
          onChange={setMaxLHSAttributes}
          step={1}
          validatorFunc={maxLHSValidatorFunc}
        />
      </div>

      <Submit
        text="Analyze"
        onClick={() => {
          onSubmit();
          sendData(file, "createTask");
        }}
        validatorFunc={isValid}
      />
    </form>
  );
}

export default FileForm;
