import React from "react";
import "./SearchBar.css";
import Button from "./Button";

function SearchBar({ defaultText, setSearchString, onClick }) {
  return (
    <div className="search-bar">
      <input
        type="text"
        className="round-corners search-input"
        size={20}
        placeholder={defaultText}
        onChange={(e) => setSearchString(e.target.value)}
      />
      <Button
        src="/icons/search.svg"
        alt="Search"
        color="green"
        onClick={onClick}
        icon
      />
    </div>
  );
}

export default SearchBar;
