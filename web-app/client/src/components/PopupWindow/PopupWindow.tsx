import React from "react";

import "./PopupWindow.scss";

interface Props {
  children: React.ReactNode;
  disable: () => void;
}

const PopupWindow: React.FC<Props> = ({ children, disable }) => (
  <div className="popup-window-bg" onClick={disable}>
    <div className="popup-window" onClick={() => {}}>
      {children}
    </div>
  </div>
);

export default PopupWindow;
