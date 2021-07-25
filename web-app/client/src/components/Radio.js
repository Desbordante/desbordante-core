import React from "react";
import "./Radio.css";

function Checkbox({ text, onClick, toggleObj }) {
  const deciderFunc = () => text === toggleObj;

  return (
    <input
      type="button"
      value={text}
      className={
        "round-corners button " + (deciderFunc() ? "checked" : "unchecked")
      }
      onClick={() => onClick(text)}
    ></input>
  );
}

export default Checkbox;
