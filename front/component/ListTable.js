import { html } from 'htm/preact';

export function ListTable({ title, icon, addLink, columns, items, renderRow }) {
  return html`
    <h2>${title}</h2>
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
        ${items?.map(renderRow)}
      </tbody>
    </table>
  `;
}
