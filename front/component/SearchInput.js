import { html } from 'htm/preact';
import { useState, useRef, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { Icon } from './Icon.js';

export function SearchInput({ onSearch }) {
  const { path, query: locationQuery, route } = useLocation();
  const [query, setQuery] = useState('');
  const inputRef = useRef(null);

  useEffect(() => {
    setQuery(locationQuery?.search || '');
    onSearch(locationQuery?.search || '');
  }, [locationQuery]);

  function routeSearch(value) {
    const params = new URLSearchParams(locationQuery);
    value ? params.set('search', value) : params.delete('search');
    const paramQuery = params.size ? `?${params}` : '';
    route(`${path}${paramQuery}`, /*replace*/ true);
  }

  function handleInput(e) {
    const value = e.target.value;
    setQuery(value);
    onSearch(value);
    routeSearch(value);
  }

  function handleClear() {
    setQuery('');
    onSearch('');
    routeSearch('');
    inputRef.current?.focus();
  }

  return html`
    <div class="input-search form">
      <input ref=${inputRef} type="text" name="search" class="input" placeholder="Search" value=${query} onInput=${handleInput} />
      ${query && html` <span class="search-field-icon" onClick=${handleClear} aria-label="Clear search"> <${Icon} name="x-circle" /> </span> `}
    </div>
  `;
}
