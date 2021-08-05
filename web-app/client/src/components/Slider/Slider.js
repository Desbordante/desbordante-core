import React from "react";
import "./Slider.css";

function Slider({
  min = 0,
  max = 1,
  onChange,
  toggleObj,
  step = 0.1,
  exponential = false,
}) {
  const exp = Math.log(10) / Math.log(2);
  const linearToExp = (numberLinear) => (exponential ? numberLinear ** exp : numberLinear);
  const expToLinear = (numberExp) => (exponential ? numberExp ** (1 / exp) : numberExp);

  return (
    <input
      type="range"
      min={min}
      max={max}
      value={+toggleObj ? expToLinear(+toggleObj) : 0}
      step={step}
      className="slider"
      onChange={(e) => onChange(
        `${((x) => (step === 1 ? parseInt(x, 10) : +x))(
          linearToExp(+e.target.value).toFixed(3),
        )}`,
      )}
    />
  );
}

export default Slider;
