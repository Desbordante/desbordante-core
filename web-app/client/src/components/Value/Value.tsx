import React, { useState, useEffect } from "react";
import "./Value.scss";

/* eslint-disable no-unused-vars */
interface Props {
  value: string;
  onChange: (str: string) => void;
  inputValidator: (str: string) => boolean;
  size?: number;
}
/* eslint-enable no-unused-vars */

const Value: React.FC<Props> = ({
  value,
  onChange,
  inputValidator,
  size = 5,
}) => {
  const [isValid, setIsValid] = useState(inputValidator(value));

  const inputHandler = (str: string) => {
    const croppedStr = str.slice(0, size);
    setIsValid(inputValidator(croppedStr));
    onChange(croppedStr);
  };

  useEffect(() => inputHandler(value), [value]);

  return (
    <input
      type="text"
      value={value}
      className={`value ${isValid ? "" : "invalid"}`}
      size={size}
      onChange={(event) => {
        // eslint-disable-next-line no-console
        inputHandler(event.target.value);
      }}
    />
  );
};

export default Value;
