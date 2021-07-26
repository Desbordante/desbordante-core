export const serverURL = "http://localhost:5000";

export async function getData(property) {
  const response = await fetch(`${serverURL}/${property}`);
  const data = await response.json();
  // console.log(data);
  return data;
}

export function sendData(file, name) {
  const formData = new FormData();
  formData.append("myFile", file);

  fetch(`${serverURL}/${name}`, {
    method: "POST",
    // headers: {
    //   "content-type": "application/json",
    // },
    body: formData,
  })
    .then((response) => response.json())
    .then((data) => {
      // eslint-disable-next-line no-console
      console.log(data);
    })
    .catch((error) => {
      // eslint-disable-next-line no-console
      console.error(error);
    });
}
