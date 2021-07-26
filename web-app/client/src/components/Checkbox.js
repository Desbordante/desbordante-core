import React from "react";
import "./Checkbox.css";

function Checkbox({ text, onClick, toggleObj }) {
  return (
    <input
      type="button"
      value={text}
      className={
        `round-corners button ${
          toggleObj ? "checked" : "unchecked"}`
      }
      onClick={() => {
        onClick(!toggleObj);
      }}
    />
  );
}

export default Checkbox;
