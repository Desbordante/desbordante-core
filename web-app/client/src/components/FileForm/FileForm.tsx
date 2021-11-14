import React, { useState, useEffect } from "react";
import axios, { AxiosResponse } from "axios";
import { useHistory } from "react-router-dom";

import "./FileForm.scss";
import Value from "../Value/Value";
import Toggle from "../Toggle/Toggle";
import Button from "../Button/Button";
import Slider from "../Slider/Slider";
import UploadFile from "../UploadFile/UploadFile";
import PopupWindow from "../PopupWindow/PopupWindow";
import FormItem from "../FormItem/FormItem";
import {
  serverURL,
  submitCustomDataset,
  submitBuiltinDataset,
} from "../../APIFunctions";
import { algorithm } from "../../types";

/* eslint-disable no-unused-vars */
/* eslint-disable no-console */
/* eslint-disable max-len */
/* eslint-disable no-shadow */
/* eslint-disable implicit-arrow-linebreak */
/* eslint-disable comma-dangle */
interface Props {
  onSubmit: (e: React.MouseEvent<HTMLButtonElement>) => void;
  setUploadProgress: (n: number) => void;
  handleResponse: (res: AxiosResponse) => void;
  file: File | null;
  setFile: (file: File | null) => void;
}

const FileForm: React.FC<Props> = ({
  onSubmit,
  setUploadProgress,
  handleResponse,
  file,
  setFile,
}) => {
  // Allowed field values
  const [allowedBuiltinDatasets, setAllowedBuiltinDatasets] = useState<
    string[]
  >([]);
  const [allowedFileFormats, setAllowedFileFormats] = useState<string[]>([
    "text/csv",
    "application/vnd.ms-excel",
  ]);
  const [allowedSeparators, setAllowedSeparators] = useState<string[]>([]);
  const [allowedAlgorithms, setAllowedAlgorithms] = useState<algorithm[]>([]);
  const [maxfilesize, setMaxFileSize] = useState(5e7);

  // Parameters, later sent to the server on execution as JSON

  const [hasHeader, setHasHeader] = useState(true);
  const [separator, setSeparator] = useState("");
  const [algorithm, setAlgorithm] = useState<algorithm | null>(null);
  const [errorThreshold, setErrorThreshold] = useState<string>("0.05");
  const [maxLHSAttributes, setMaxLHSAttributes] = useState<string>("5");
  const [threadsCount, setThreadsCount] = useState<string>("1");

  // Builtin datasets
  const [isWindowShown, setIsWindowShown] = useState(false);
  const [builtinDataset, setBuiltinDataset] = useState<string | null>(null);

  const history = useHistory();

  // Getting allowed field values from server
  useEffect(() => {
    axios
      .get(`${serverURL}/algsInfo`, { timeout: 2000 })
      .then((res) => res.data)
      .then((data) => {
        setAllowedFileFormats(data.allowedFileFormats);

        setAllowedBuiltinDatasets(data.availableDatasets);

        setAllowedAlgorithms(data.algorithmsInfo);
        setAlgorithm(data.algorithmsInfo[0]);

        setAllowedSeparators(data.allowedSeparators);
        setSeparator(data.allowedSeparators[0]);

        setMaxFileSize(data.maxFileSize);
      })
      .catch((error) => history.push("/error"));
  }, [history]);

  useEffect(() => {
    if (!algorithm?.props.errorThreshold) {
      setErrorThreshold("0");
    }
    if (!algorithm?.props.maxLHS) {
      setMaxLHSAttributes("inf");
    }
    if (!algorithm?.props.threads) {
      setThreadsCount("1");
    }
  }, [algorithm]);

  // Validator functions for fields
  const fileExistenceValidator = (file: File | null) => !!file;
  const fileSizeValidator = (file: File | null) =>
    !!file && file.size <= maxfilesize;
  const fileFormatValidator = (file: File | null) =>
    !!file && allowedFileFormats.indexOf(file.type) !== -1;

  const separatorValidator = (sep: string) =>
    allowedSeparators.indexOf(sep) !== -1;
  const errorValidator = (err: string) =>
    !Number.isNaN(+err) && +err >= 0 && +err <= 1;
  const maxLHSValidator = (lhs: string) =>
    lhs === "inf" || (!Number.isNaN(+lhs) && +lhs > 0 && +lhs % 1 === 0);

  // Validator function that ensures every field is correct
  function isValid() {
    return (
      (!!builtinDataset ||
        (fileExistenceValidator(file) &&
          fileSizeValidator(file) &&
          fileFormatValidator(file))) &&
      separatorValidator(separator) &&
      errorValidator(errorThreshold) &&
      maxLHSValidator(maxLHSAttributes)
    );
  }

  const submit = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    e.preventDefault();
    const sendAlgName = algorithm ? algorithm.name : allowedAlgorithms[0].name;
    const sendErrorThreshold = +errorThreshold;
    const sendMaxLHS = maxLHSAttributes === "inf" ? -1 : +maxLHSAttributes;
    if (builtinDataset) {
      submitBuiltinDataset(
        builtinDataset!,
        {
          algName: sendAlgName,
          errorPercent: sendErrorThreshold,
          maxLHS: sendMaxLHS,
          parallelism: threadsCount,
        },
        handleResponse
      );
    } else {
      history.push("/loading");
      submitCustomDataset(
        file as File,
        {
          algName: sendAlgName,
          separator,
          errorPercent: sendErrorThreshold,
          hasHeader,
          maxLHS: sendMaxLHS,
          parallelism: threadsCount,
        },
        setUploadProgress,
        handleResponse
      );
    }
  };

  return (
    <form>
      {isWindowShown && (
        <PopupWindow disable={() => setIsWindowShown(false)}>
          {allowedBuiltinDatasets.map((datasetName) => (
            <Toggle
              toggleCondition={builtinDataset === datasetName}
              onClick={() => {
                setFile(null);
                setBuiltinDataset(datasetName);
                setIsWindowShown(false);
              }}
              color="1"
              key={datasetName}
            >
              {datasetName}
            </Toggle>
          ))}
        </PopupWindow>
      )}
      <div className="form-column">
        <div className="form-row">
          <FormItem>
            <UploadFile
              onClick={setFile}
              file={file}
              builtinDataset={builtinDataset}
              fileExistenceValidator={() => fileExistenceValidator(file)}
              fileSizeValidator={() => fileSizeValidator(file)}
              fileFormatValidator={() => fileFormatValidator(file)}
              openPopupWindow={() => setIsWindowShown(true)}
              disableBuiltinDataset={() => setBuiltinDataset(null)}
            />
          </FormItem>
          <FormItem enabled={!builtinDataset}>
            <h3>File properties:</h3>
            <Toggle
              color="1"
              onClick={() => setHasHeader(!hasHeader)}
              toggleCondition={hasHeader}
            >
              Header
            </Toggle>
            <h3>separator</h3>
            <Value
              value={separator}
              onChange={setSeparator}
              size={2}
              inputValidator={separatorValidator}
            />
          </FormItem>
          <FormItem enabled={algorithm?.props.errorThreshold}>
            <h3>Error threshold:</h3>
            <Value
              value={errorThreshold}
              onChange={setErrorThreshold}
              size={4}
              inputValidator={errorValidator}
            />
            <Slider
              value={errorThreshold}
              onChange={setErrorThreshold}
              step={0.001}
              exponential
            />
          </FormItem>
          <FormItem enabled={algorithm?.props.maxLHS}>
            <h3>Max LHS attributes:</h3>
            <Value
              value={maxLHSAttributes}
              onChange={setMaxLHSAttributes}
              size={3}
              inputValidator={maxLHSValidator}
            />
            <Slider
              value={maxLHSAttributes === "inf" ? "10" : maxLHSAttributes}
              min={1}
              max={10}
              onChange={setMaxLHSAttributes}
              step={1}
            />
          </FormItem>

          <FormItem enabled={algorithm?.props.threads}>
            <h3>Threads:</h3>
            <Value
              value={threadsCount}
              onChange={setThreadsCount}
              size={2}
              inputValidator={maxLHSValidator}
            />
            <Slider
              value={threadsCount}
              min={1}
              max={16}
              onChange={setThreadsCount}
              step={1}
            />
          </FormItem>
        </div>

        <div className="form-row">
          <FormItem>
            <h3>Algorithm:</h3>
            {allowedAlgorithms.map((algo) => (
              <Toggle
                color="1"
                onClick={() => setAlgorithm(algo)}
                toggleCondition={algorithm === algo}
                key={algo.name}
              >
                {algo.name}
              </Toggle>
            ))}
          </FormItem>
        </div>
      </div>

      <div className="form-column">
        <Button color="1" enabled={isValid()} onClick={submit}>
          Analyze
        </Button>
      </div>
    </form>
  );
};

export default FileForm;
