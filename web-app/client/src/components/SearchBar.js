import React from "react";
import "./SearchBar.css";

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
    </div>
  );
}

export default SearchBar;
