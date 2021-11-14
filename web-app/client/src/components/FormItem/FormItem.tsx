import React from "react";

import "./FormItem.scss";

interface Props {
  enabled?: boolean;
}

const FormItem: React.FC<Props> = ({ enabled = true, children }) => (
  <div className="form-item">
    {!enabled && <div className="shadow" />}
    {children}
  </div>
);

export default FormItem;
