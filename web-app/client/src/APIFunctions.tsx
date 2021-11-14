import axios, { AxiosResponse } from "axios";

export const serverURL = "http://localhost:5000";

/* eslint-disable no-console */
/* eslint-disable comma-dangle */

export async function getData(property: string) {
  try {
    const response = await axios.get(`${serverURL}/${property}`);
    return response.data;
  } catch (e) {
    return e;
  }
}

type parameters = {
  algName: string;
  separator?: string;
  errorPercent: number;
  hasHeader?: boolean;
  maxLHS: number;
  parallelism: string;
};

export function submitBuiltinDataset(
  dataset: string,
  params: parameters,
  /* eslint-disable-next-line no-unused-vars */
  onComplete: (res: AxiosResponse) => void
) {
  const json = JSON.stringify(params);
  const blob = new Blob([json], {
    type: "application/json",
  });

  const data = new FormData();
  data.append("fileName", dataset);
  data.append("document", blob);

  axios.post(`${serverURL}/chooseTask`, data).then((response) => {
    onComplete(response);
  });
}

export function submitCustomDataset(
  dataSet: File,
  params: parameters,
  /* eslint-disable-next-line no-unused-vars */
  onProgress: (n: number) => void,
  /* eslint-disable-next-line no-unused-vars */
  onComplete: (res: AxiosResponse) => void
) {
  const json = JSON.stringify(params);
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
    onUploadProgress: (progressEvent: ProgressEvent) => {
      onProgress(progressEvent.loaded / progressEvent.total);
    },
  };

  axios.post(`${serverURL}/createTask`, data, config).then((response) => {
    onComplete(response);
  });
}
