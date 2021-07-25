import React from "react";
import "./Submit.css";

function Submit({ text, onClick, toggleObj, validatorFunc }) {
  // console.log(validatorFunc(toggleObj));

  return (
    <div className="gradient-glow">
      <div
        className={
          "round-corners button gradient-fill glow" +
          (validatorFunc() ? "" : " disabled")
        }
      >
        {text}
      </div>
      <input
        type="button"
        className={
          "round-corners button gradient-fill" +
          (validatorFunc() ? " button" : " disabled")
        }
        onClick={validatorFunc() ? onClick : () => {}}
        value={text}
      ></input>
    </div>
  );
}

export default Submit;
