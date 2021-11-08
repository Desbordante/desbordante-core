import React from "react";
import "./SelectedAttribute.scss";

/* eslint-disable no-unused-vars */
interface Props {
  text: string;
  onDelete: () => void;
}
/* eslint-enable no-unused-vars */

const SelectedAttribute: React.FC<Props> = ({ text, onDelete }) => (
  <div className="selected-attribute">
    {text}
    <button type="button" onClick={onDelete}>
      <img src="/icons/cross.svg" alt="d" />
    </button>
  </div>
);

export default SelectedAttribute;
