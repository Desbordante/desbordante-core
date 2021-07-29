import axios from "axios";

const serverURL = "http://localhost:5000";

export async function getData(property) {
  const response = await fetch(`${serverURL}/${property}`);
  const data = await response.json();
  // console.log(data);
  return data;
}

export function submitDatasetWthParameters(
  dataSet,
  parameters,
  onProgress,
  cancelTokenSource,
) {
  const json = JSON.stringify(parameters);
  const blob = new Blob([json], {
    type: "application/json",
  });

  const data = new FormData();

  // Update the formData object
  if (dataSet) {
    data.append("file", dataSet, dataSet.name);
  }

  data.append("document", blob);

  // Request made to the backend api
  // Send formData object
  const config = {
    headers: { "Content-Type": "text/csv" },
    onUploadProgress: (progressEvent) => onProgress(progressEvent.loaded / progressEvent.total),
    cancelToken: cancelTokenSource.token,
  };

  axios.post(`${serverURL}/createTask`, data, config);
}
