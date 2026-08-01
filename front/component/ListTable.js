import { html } from 'htm/preact';
import { useEffect, useState, useMemo } from 'preact/hooks';
import { useSearch } from '../hook/useSearch.js';
import { SearchInput } from './SearchInput.js';

export function ListTable({ title, icon, addLink, columns, items, searchKeys, renderRow }) {
  const { results, search } = useSearch(items, searchKeys);
  const [sort, setSort] = useState({ key: null, dir: 1 });

  const sorted = useMemo(() => {
    if (!sort.key || !results) return results;
    return [...results].sort((a, b) => {
      const va = a[sort.key];
      const vb = b[sort.key];
      if (va == null) return 1;
      if (vb == null) return -1;
      return String(va).localeCompare(String(vb), undefined, { numeric: true }) * sort.dir;
    });
  }, [results, sort]);

  function toggleSort(key) {
    if (!key) return;
    setSort((s) => (s.key === key ? { key, dir: -s.dir } : { key, dir: 1 }));
  }

  useEffect(() => {
    toggleSort(columns.find((c) => c?.key)?.key);
  }, []);

  return html`
    <h2>${title}</h2>
    <div class="pure-g">
      <div class="search-content pure-u-1 pure-u-md-1-2 pure-u-lg-1-3">
        <${SearchInput} onSearch=${search} />
      </div>
    </div>

    <table class="table pure-table pure-table-horizontal">
      <thead>
        <tr>
          ${columns.map(
            (col, i) =>
              html`<th colspan="${i === columns.length - 1 ? 2 : 1}">
                <div class="table-header-content">
                  <div class="table-sort-button" onClick=${() => toggleSort(col?.key)}>
                    ${col?.label}
                    ${
                      col?.key &&
                      html` <svg
                        class="table-sort-icon table-sorted-${sort.key === col.key ? (sort.dir === 1 ? 'asc' : 'desc') : 'asc'}"
                        xmlns="http://www.w3.org/2000/svg"
                        fill="currentColor"
                        viewBox="0 0 16 16"
                      >
                        <path d="M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z" />
                      </svg>`
                    }
                  </div>

                  ${
                    i === columns.length - 1 &&
                    html`
                      <a class="table-add-button" href="${addLink}">
                        <svg class="svg-button">${icon}</svg>
                      </a>
                    `
                  }
                </div>
              </th>`
          )}
        </tr>
      </thead>
      <tbody>
        ${sorted?.map(renderRow)}
      </tbody>
    </table>
  `;
}
