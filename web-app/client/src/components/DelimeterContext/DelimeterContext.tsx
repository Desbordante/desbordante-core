import React, { createContext, useState } from "react";

type DelimeterContextType = {
  delimeter: string,
  setDelimeter: React.Dispatch<React.SetStateAction<string>>
}

type DelimeterContextProviderProps = {
  children: React.ReactNode
}

export const DelimeterContext = createContext<DelimeterContextType | null>(null);

export const DelimeterContextProvider = ({ children }: DelimeterContextProviderProps) => {
  const [delimeter, setDelimeter] = useState<string>("");
  return (
    <DelimeterContext.Provider value={{ delimeter, setDelimeter }}>
      {children}
    </DelimeterContext.Provider>
  );
};
