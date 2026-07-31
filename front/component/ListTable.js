import { html } from 'htm/preact';
import { useSearch } from '../hook/useSearch.js';
import { SearchInput } from './SearchInput.js';

export function ListTable({ title, icon, addLink, columns, items, searchKeys, renderRow }) {
  const { results, search } = useSearch(items, searchKeys);

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
          ${columns.map((col) => html`<th>${col}</th>`)}
          <th class="add-user">
            <a href="${addLink}">
              <svg class="svg-button">${icon}</svg>
            </a>
          </th>
        </tr>
      </thead>
      <tbody>
        ${results?.map(renderRow)}
      </tbody>
    </table>
  `;
}
