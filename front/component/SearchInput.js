import { html } from 'htm/preact';
import { useState, useRef } from 'preact/hooks';

export function SearchInput({ onSearch }) {
  const [query, setQuery] = useState('');
  const inputRef = useRef(null);

  function handleInput(e) {
    const value = e.target.value;
    setQuery(value);
    onSearch(value);
  }

  function handleClear() {
    setQuery('');
    onSearch('');
    inputRef.current?.focus();
  }

  const xCircle = html`
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="black" class="bi bi-x-circle" viewBox="0 0 16 16">
      <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16" />
      <path
        d="M4.646 4.646a.5.5 0 0 1 .708 0L8 7.293l2.646-2.647a.5.5 0 0 1 .708.708L8.707 8l2.647 2.646a.5.5 0 0 1-.708.708L8 8.707l-2.646 2.647a.5.5 0 0 1-.708-.708L7.293 8 4.646 5.354a.5.5 0 0 1 0-.708"
      />
    </svg>
  `;

  return html`
    <div class="form-search pure-form">
      <input
        ref=${inputRef}
        type="text"
        name="search"
        class="pure-input-rounded pure-input-1"
        placeholder="Search"
        value=${query}
        onInput=${handleInput}
      />
      ${query && html` <span class="search-field-icon" onClick=${handleClear} aria-label="Clear search"> ${xCircle} </span> `}
    </div>
  `;
}
