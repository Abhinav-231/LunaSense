import { createContext, useContext, useState } from "react";

const LunaContext = createContext({
  espIP: "192.168.1.50",
  setEspIP: (ip: string) => {},
});

export const LunaProvider = ({ children }: any) => {
  const [espIP, setEspIP] = useState("192.168.1.50");
  return (
    <LunaContext.Provider value={{ espIP, setEspIP }}>
      {children}
    </LunaContext.Provider>
  );
};
export const useLuna = () => useContext(LunaContext);
