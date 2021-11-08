import React from "react";

import "./Phasename.scss";

interface Props {
  phaseName: string;
  progress: number;
  currentPhase: number;
  maxPhase: number;
}

const Phasename: React.FC<Props> = ({
  phaseName,
  progress,
  currentPhase,
  maxPhase,
}) => {
  const rem = parseFloat(getComputedStyle(document.documentElement).fontSize);
  const clamp = (number: number, min: number, max: number) =>
    Math.min(max, Math.max(min, number));

  const message = `Phase ${currentPhase} of ${maxPhase}: ${phaseName}...`;

  const pointerWidth = 3 * rem;
  const titleWidth = message.length * 0.65 * rem;
  const titleBorderRadius = 1 * rem;
  const margin = 1 * rem;

  const availableWidth = window.innerWidth;

  const titleTransform = clamp(
    availableWidth * progress - titleWidth / 2,
    margin,
    availableWidth - titleWidth - margin
  );

  const pointerTransform = clamp(
    availableWidth * progress - pointerWidth / 2,
    margin + titleBorderRadius,
    availableWidth - pointerWidth - titleBorderRadius - margin
  );

  let opacity = 1;
  if (
    availableWidth * progress <
    margin + titleBorderRadius + pointerWidth / 2
  ) {
    opacity =
      (availableWidth * progress) /
      (margin + titleBorderRadius + pointerWidth / 2);
  }
  if (
    availableWidth * progress >
    availableWidth - pointerWidth / 2 - titleBorderRadius - margin
  ) {
    opacity =
      (availableWidth * (1 - progress)) /
      (pointerWidth / 2 + titleBorderRadius + margin);
  }

  return (
    <div
      className="phase-name-container"
      style={{
        marginTop: margin / 2,
        opacity,
      }}
    >
      <img
        src="/icons/progressbar_pointer.svg"
        alt="I"
        style={{
          transform: `translateX(${pointerTransform}px)`,
          width: pointerWidth,
        }}
      />
      <div
        className="phase-name"
        style={{
          transform: `translateX(${titleTransform}px)`,
          width: titleWidth,
          borderRadius: titleBorderRadius,
        }}
      >
        {message}
      </div>
    </div>
  );
};

export default Phasename;
