import React from "react";

import "./Dependency.css";
import { coloredDepedency } from "../../types";

interface Props {
  dep: coloredDepedency | undefined;
  isActive: boolean;
  onClick: React.MouseEventHandler<HTMLDivElement>;
}

const Dependency: React.FC<Props> = ({ dep, isActive, onClick }) => {
  if (dep !== undefined) {
    return (
      <div className="dependency" role="button" tabIndex={0} onClick={onClick}>
        {
          dep.lhs.map((attr) => (
            <div
              style={isActive ? {
                backgroundColor: `${attr.color}`,
              } : {
                backgroundColor: "#E5E5E5",
              }}
              className={`attribute-name ${isActive && "active"}`}
              key={attr.name}
            >
              {attr.name}
            </div>
          ))
        }

        <svg
          className={`arrow ${isActive ? "active" : ""}`}
          viewBox="0 0 58.73 20.09"
        >
          <line x1="48.68" y1="0.5" x2="58.23" y2="10.05" />
          <line x1="58.23" y1="10.05" x2="48.68" y2="19.59" />
          <line x1="58.23" y1="10.05" x2="0.5" y2="10.05" />
        </svg>

        <div
          style={isActive ? {
            backgroundColor: `${dep.rhs.color}`,
          } : { backgroundColor: "#E5E5E5" }}
          className={`attribute-name ${isActive ? "active" : ""}`}
        >
          {dep.rhs.name}
        </div>
      </div>
    );
  }
  return (<></>);
};

export default Dependency;
