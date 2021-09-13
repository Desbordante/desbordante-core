import React from "react";

import "./Dependency.css";
import { dependency } from "../../types";

interface Props {
  dep: dependency;
  isActive: boolean;
  onClick: React.MouseEventHandler<HTMLDivElement>;
}

const Dependency: React.FC<Props> = ({
  dep,
  isActive,
  onClick,
}) => (
  <div className="dependency" role="button" tabIndex={0} onClick={onClick}>
    {dep.lhs.map((attr) => (
      <div className={`attribute-name ${isActive && "active"}`} key={attr.name}>
        {attr.name}
      </div>
    ))}

    <svg
      className={`arrow ${isActive ? "active" : ""}`}
      viewBox="0 0 58.73 20.09"
    >
      <line x1="48.68" y1="0.5" x2="58.23" y2="10.05" />
      <line x1="58.23" y1="10.05" x2="48.68" y2="19.59" />
      <line x1="58.23" y1="10.05" x2="0.5" y2="10.05" />
    </svg>

    <div className={`attribute-name ${isActive ? "active" : ""}`}>{dep.rhs.name}</div>
  </div>
);

export default Dependency;
