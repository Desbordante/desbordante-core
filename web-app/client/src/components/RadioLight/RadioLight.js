import React from "react";
import "./RadioLight.css";

function RadioLight({ text, onClick, toggleObj }) {
  const deciderFunc = () => text === toggleObj;

  return (
    <input
      type="button"
      value={text}
      className={`round-corners button radio-light ${
        deciderFunc() ? "checked" : "unchecked"
      }`}
      onClick={() => onClick(text)}
    />
  );
}

export default RadioLight;
