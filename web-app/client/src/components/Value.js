import React from "react";
import "./Value.css";

function Value({ toggleObj, onChange, size, validatorFunc }) {
  return (
    <input
      type="text"
      value={toggleObj}
      className={
        "round-corners unchecked value" +
        (validatorFunc(toggleObj) ? "" : " invalid")
      }
      size={size}
      onInput={(e) => onChange(e.target.value)}
    />
  );
}

export default Value;
