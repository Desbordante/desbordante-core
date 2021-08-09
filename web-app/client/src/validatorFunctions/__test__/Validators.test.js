import * as funcs from "../Validators";

// fileExistence
test("fileExistence handles null", () => {
  expect(funcs.fileExistence(null)).toEqual(false);
});
test("fileExistence handles undefined", () => {
  expect(funcs.fileExistence(undefined)).toEqual(false);
});
test("fileExistence handles file", () => {
  expect(funcs.fileExistence({ name: "QWE" })).toEqual(true);
});

// fileSize
test("fileSize handles null", () => {
  expect(funcs.fileSize(null, 10)).toEqual(false);
});
test("fileSize handles undefined", () => {
  expect(funcs.fileSize(undefined, 10)).toEqual(false);
});
test("fileSize handles allowed size", () => {
  expect(funcs.fileSize({ size: 9 }, 10)).toEqual(true);
});
test("fileSize handles non-allowed size", () => {
  expect(funcs.fileSize({ size: 9 }, 8)).toEqual(false);
});

// fileFormat
test("fileFormat handles null", () => {
  expect(funcs.fileFormat(null, ["a", "b"])).toEqual(false);
});
test("fileFormat handles undefined", () => {
  expect(funcs.fileFormat(undefined, ["a", "b"])).toEqual(false);
});
test("fileFormat handles correct format", () => {
  expect(funcs.fileFormat({ type: "a" }, ["a", "b"])).toEqual(
    true,
  );
});
test("fileFormat handles correct format", () => {
  expect(funcs.fileFormat({ type: "c" }, ["a", "b"])).toEqual(
    false,
  );
});

// separator
test("separator handles null", () => {
  expect(funcs.separator(null, ["a", "b"])).toEqual(false);
});
test("separator handles undefined", () => {
  expect(funcs.separator(undefined, ["a", "b"])).toEqual(false);
});
test("separator handles correct format", () => {
  expect(funcs.separator("a", ["a", "b"])).toEqual(true);
});
test("separator handles correct format", () => {
  expect(funcs.separator("c", ["a", "b"])).toEqual(false);
});

// error
test("error handles null", () => {
  expect(funcs.error(null)).toEqual(false);
});
test("error handles NaN", () => {
  expect(funcs.error(NaN)).toEqual(false);
});
test("error handles undefined", () => {
  expect(funcs.error(undefined)).toEqual(false);
});
test("error handles correct format", () => {
  expect(funcs.error(0.5)).toEqual(true);
});
test("error handles 1", () => {
  expect(funcs.error(1)).toEqual(true);
});
test("error handles 0", () => {
  expect(funcs.error(0)).toEqual(true);
});
test("error handles negatives", () => {
  expect(funcs.error(-1)).toEqual(false);
});
test("error handles overflow", () => {
  expect(funcs.error(2)).toEqual(false);
});

// maxLHS
test("maxLHS handles null", () => {
  expect(funcs.maxLHS(null)).toEqual(false);
});
test("maxLHS handles undefined", () => {
  expect(funcs.maxLHS(undefined)).toEqual(false);
});
test("maxLHS handles NaN", () => {
  expect(funcs.maxLHS(NaN)).toEqual(false);
});
test("maxLHS handles non-integers", () => {
  expect(funcs.maxLHS(0.5)).toEqual(false);
});
test("maxLHS handles negative integers", () => {
  expect(funcs.maxLHS(-1)).toEqual(false);
});
test("maxLHS handles positive integers", () => {
  expect(funcs.maxLHS(1)).toEqual(true);
});
