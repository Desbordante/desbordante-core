import React from "react";
import "./AttributeLabel.scss";

interface Props {
  text: string;
  labelColor: string;
}

const AttributeLabel: React.FC<Props> = ({ text, labelColor }) => (
  <div className="attribute-label">
    <div className="circle" style={{ backgroundColor: labelColor }} />
    {text}
  </div>
);

export default AttributeLabel;
