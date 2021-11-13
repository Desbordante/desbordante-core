export type attribute = { name: string; value: number };
export type dependencyEncoded = { lhs: number[]; rhs: number };
export type dependency = { lhs: attribute[]; rhs: attribute };
export type taskStatus =
  | "UNSCHEDULED"
  | "PROCESSING"
  | "COMPLETED"
  | "SERVER ERROR"
  | "INCORRECT INPUT DATA";
export type parameters = {
  algName: string;
  separator: string;
  errorPercent: number;
  hasHeader: boolean;
  maxLHS: number;
};
export type algorithm = {
  name: string;
  props: {
    errorThreshold: boolean;
    maxLHS: boolean;
    threads: boolean;
  };
};
export type coloredDepedency = { lhs: coloredAttribute[]; rhs: coloredAttribute };
export type coloredAttribute = { name: string, value: number, color: string };
