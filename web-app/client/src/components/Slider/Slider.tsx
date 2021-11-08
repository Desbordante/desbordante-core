import React from "react";
import "./Slider.scss";

/* eslint-disable no-unused-vars */
interface Props {
  value: string;
  onChange: (str: string) => void;
  min?: number;
  max?: number;
  step?: number;
  exponential?: boolean;
}
/* eslint-enable no-unused-vars */

const Slider: React.FC<Props> = ({
  value,
  onChange,
  min = 0,
  max = 1,
  step = 0.005,
  exponential = false,
}) => {
  const exp = Math.log(10) / Math.log(2);
  const linearToExp = (numberLinear: number) => numberLinear ** (1 / exp);
  const expToLinear = (numberExp: number) => numberExp ** exp;
  const stepFunction = (x: number) => step * Math.floor(x / step);

  const changeHandler = (str: string) => {
    let newValue = exponential ? expToLinear(+str) : +str;
    newValue = stepFunction(newValue);

    onChange(newValue.toString());
  };

  return (
    <input
      type="range"
      min={exponential ? linearToExp(min) : min}
      max={exponential ? linearToExp(max) : max}
      value={exponential ? linearToExp(+value) : +value}
      step={step}
      className="slider"
      onChange={(e) => changeHandler(e.target.value)}
    />
  );
};

export default Slider;
