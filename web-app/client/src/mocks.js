window.HTMLElement.prototype.scrollIntoView = function() {};

jest.mock("react-chartjs-2", () => ({
  Doughnut: () => null,
}));

Object.defineProperty(HTMLMediaElement.prototype, "muted", {
  set: () => {},
});
