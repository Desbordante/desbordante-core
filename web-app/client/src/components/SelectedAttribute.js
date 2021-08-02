import React from "react";
import "./SelectedAttribute.css";

function SelectedAttribute({ text, onDelete }) {
  return (
    <div className="attribute">
      {text}
      <button type="button" onClick={onDelete}>
        <img
          src="/icons/cross.svg"
          alt="d"
          style={{ width: "0.3rem", height: "0.3rem" }}
        />
      </button>
    </div>
  );
}

export default SelectedAttribute;
