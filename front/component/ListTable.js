import { html } from 'htm/preact';
import { useEffect, useState, useMemo } from 'preact/hooks';
import { useSearch } from '../hook/useSearch.js';
import { SearchInput } from './SearchInput.js';
import { Icon } from './Icon.js';

function ChevronRightIcon({ class: className }) {
  return html`<${Icon} name="chevron-right" class="${className}" />`;
}

export function ListTable({ title, addContent, addLink, columns, items, searchKeys, idKey = 'id', renderExpanded }) {
  const { results, search } = useSearch(items, searchKeys);
  const [sort, setSort] = useState({ col: null, dir: 1 });
  const [expanded, setExpanded] = useState(new Set());

  const sorted = useMemo(() => {
    if (!sort.col || !results) return results;
    return [...results].sort((a, b) => {
      const va = sort.col.sortValue ? sort.col.sortValue(a) : a[sort.col.key];
      const vb = sort.col.sortValue ? sort.col.sortValue(b) : b[sort.col.key];
      if (va == null) return 1;
      if (vb == null) return -1;
      return String(va).localeCompare(String(vb), undefined, { numeric: true }) * sort.dir;
    });
  }, [results, sort]);

  function toggleSort(col) {
    if (!col) return;
    // since first col is desc, new col must be asc
    setSort((s) => ({ col, dir: -s.dir }));
  }

  useEffect(() => {
    // sort first col desc
    const col = columns.find((c) => c?.key);
    setSort({ col, dir: 1 });
  }, []);

  function toggleRow(id) {
    setExpanded((prev) => {
      const next = new Set(prev);
      next.has(id) ? next.delete(id) : next.add(id);
      return next;
    });
  }

  function toggleAll() {
    setExpanded((prev) => (prev.size ? new Set() : new Set(sorted.map((i) => i[idKey]))));
  }

  const primaryColumns = columns.filter((c) => c.primary !== false);

  return html`
    <h2>${title}</h2>
    <div class="list-table-toolbar">
      <div class="list-search">
        <${SearchInput} onSearch=${search} />
      </div>

      <div class="list-toolbar">
        <a class="list-collapse-toggle" onClick=${toggleAll}>${expanded.size ? 'Collapse all' : 'Expand all'}</a>
        <a class="list-add-button" href="${addLink}"> ${addContent} </a>
      </div>
    </div>

    <div class="list-table">
      <div class="list-header">
        ${primaryColumns.map(
          (col) => html`
            <div class="list-header-cell" onClick=${() => toggleSort(col)}>
              ${col.label}
              <${Icon}
                name="chevron-down"
                class="list-sort-icon list-sorted-${sort.col?.key === col.key ? (sort.dir === 1 ? 'asc' : 'desc') : 'asc'}"
              />
            </div>
          `
        )}
      </div>

      ${sorted?.map((item) => {
        const id = item[idKey];
        const isOpen = expanded.has(id);
        const expandedContent = renderExpanded(item);
        return html`
          <div class="list-row" key=${id}>
            <div class="list-row-summary ${!expandedContent ? 'is-disabled' : ''}" onClick=${() => expandedContent && toggleRow(id)}>
              ${primaryColumns.map((col) => {
                const itemContent = col.render ? col.render(item) : item[col.key];
                return itemContent && html`<div class="list-row-cell">${itemContent}</div>`;
              })}
              ${expandedContent && html`<${ChevronRightIcon} class="list-row-chevron ${isOpen ? 'is-open' : ''}" />`}
            </div>
            ${isOpen && expandedContent && html`<div class="list-row-expanded">${expandedContent}</div>`}
          </div>
        `;
      })}
    </div>
  `;
}
