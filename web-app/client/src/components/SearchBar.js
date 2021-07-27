import React from "react";
import "./SearchBar.css";
import ButtonIcon from "./ButtonIcon";

function SearchBar({ defaultText, setSearchString }) {
  return (
    <div className="search-bar">
      <input
        type="text"
        className="round-corners search-input"
        size={20}
        placeholder={defaultText}
        onChange={(e) => setSearchString(e.target.value)}
      />
      <ButtonIcon src="/icons/search.svg" alt="Search" color="green" />
    </div>
  );
}

export default SearchBar;
