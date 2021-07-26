import React from "react";
import "./Submit.css";

function Submit({
  // eslint-disable-next-line no-unused-vars
  text, onClick, toggleObj, validatorFunc,
}) {
  // console.log(validatorFunc(toggleObj));

  return (
    <div className="gradient-glow">
      <div
        className={
          `round-corners button gradient-fill glow${
            validatorFunc() ? "" : " disabled"}`
        }
      >
        {text}
      </div>
      <input
        type="button"
        className={
          `round-corners button gradient-fill${
            validatorFunc() ? " button" : " disabled"}`
        }
        onClick={validatorFunc() ? onClick : () => {}}
        value={text}
      />
    </div>
  );
}

export default Submit;
