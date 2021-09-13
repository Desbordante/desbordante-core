import React from "react";
import { useHistory } from "react-router-dom";

import "./ErrorScreen.css";
import Button from "../Button/Button";

interface Props {
  code: string;
  message: string;
}

const ErrorScreen: React.FC<Props> = ({ code, message }) => {
  const history = useHistory();

  return (
    <div className="error-bg">
      <div className="error-container">
        <div className="error-message">
          {/* <img src="/icons/error.svg" alt="Error" /> */}
          <span>
            Error
            {" "}
            {code}
            :
            {" "}
          </span>
          {message}
        </div>
        <Button type="button" color="1" onClick={() => history.push("/")}>Try again</Button>
      </div>
    </div>
  );
};

export default ErrorScreen;
