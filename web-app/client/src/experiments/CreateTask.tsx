import axios from "axios";
import React, { useState } from "react";
import { Button } from "@skbkontur/react-ui";

// export const FileUpload = () => {
//   const [selectedFile, setSelectedFile] = useState<File>();
//   const [isSelected, setIsFilePicked] = useState(false);
//
//   const changeHandler = (event : React.ChangeEvent<HTMLInputElement>) => {
//     if (event && event.target && event.target.files && event.target.files[0]) {
//       setSelectedFile(event.target.files[0]);
//     }
//     setIsFilePicked(true);
//   };
//
//   const handleSubmission = () => {
//     const formData = new FormData();
//
//     formData.append("file", selectedFile!, selectedFile!.name);
//
//     fetch(
//       "http://localhost:5000/upload",
//       {
//         method: "POST",
//         headers: { "Content-Type": "multipart/form-data" }, // application/csv text/csv
//         body: formData,
//       },
//     )
//       .then((response) => response.json())
//       .then((result) => {
//         // eslint-disable-next-line no-console
//         console.log("Success:", result);
//       })
//       .catch((error) => {
//         // eslint-disable-next-line no-console
//         console.error("Error:", error);
//       });
//   };
//
//   return (
//     <div>
//       <input type="file" name="file" onChange={changeHandler} />
//       {isSelected && selectedFile ? (
//         <div>
//           <p>
//             Filename:
//             {selectedFile.name}
//           </p>
//           <p>
//             Filetype:
//             {selectedFile.type}
//           </p>
//           <p>
//             Size in bytes:
//             {selectedFile.size}
//           </p>
//           <p>
//             lastModifiedDate:
//             {" "}
//             {selectedFile.lastModified}
//           </p>
//         </div>
//       ) : (
//         <p>Select a file to show details</p>
//       )}
//       <div>
//         <button type="button" onClick={handleSubmission}>Submit</button>
//       </div>
//     </div>
//   );
// };

export const CreateTask = () => {
  const [selectedFile, setSelectedFile] = useState<File>();

  // On file select (from the pop up)
  function onFileChange(event:React.ChangeEvent<HTMLInputElement>) {
    setSelectedFile(event.target.files![0]);
  }

  // On file upload (click the upload button)
  function onFileUpload() {
    // Details of the uploaded file
    const json = JSON.stringify({ algName: "Tane", semicolon: ",", errorPercent: 0.02 });
    const blob = new Blob([json], {
      type: "application/json",
    });
    // Create an object of formData
    const data = new FormData();

    // Update the formData object
    if (selectedFile) {
      data.append(
        "file",
        selectedFile,
        selectedFile.name,
      );
    }

    data.append("document", blob);

    // Request made to the backend api
    // Send formData object
    axios({
      method: "post",
      url: "http://localhost:5000/createTask",
      data,
      headers: { "Content-Type": "text/csv" },
    });
    // axios.post("http://localhost:5000/upload", data, { headers: { "Content-Type": "text/csv" } });
  }

  // File content to be displayed after
  // file upload is complete
  function fileData() {
    if (selectedFile) {
      return (
        <div>
          <h2>File Details:</h2>
          <p>
            File Name:
            {selectedFile.name}
          </p>
          <p>
            File Type:
            {selectedFile.type}
          </p>
        </div>
      );
    }
    return (
      <div>
        <br />
        <h4>Choose before Pressing the Upload button</h4>
      </div>
    );
  }

  return (
    <div style={{ width: "400px" }}>
      <h3>
        File Upload using React!
      </h3>
      <div>
        <input type="file" onChange={onFileChange} />
        <Button type="button" onClick={onFileUpload}>
          Create new task
        </Button>
      </div>
      {fileData()}
    </div>
  );
};
