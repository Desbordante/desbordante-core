/* eslint-disable no-console */

import React from "react";
import { useHistory } from "react-router-dom";

import "./HomeScreen.css";
import Header from "../Header/Header";
import FileForm from "../FileForm/FileForm";

/* eslint-disable no-unused-vars */
interface Props {
  setUploadProgress: (n: number) => void;
}
/* eslint-enable no-unused-vars */

const HomeScreen: React.FC<Props> = ({ setUploadProgress }) => {
  const history = useHistory();

  return (
    <div className="home-screen">
      <Header />

      <FileForm
        onSubmit={() => history.push("/loading")}
        setUploadProgress={setUploadProgress}
        handleResponse={(res) => {
          history.push(`/attrs/${res.data.taskID}`);
        }}
      />
    </div>
  );
};

export default HomeScreen;
