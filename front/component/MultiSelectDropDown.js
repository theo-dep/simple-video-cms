import { html } from 'htm/preact';
import { useState, useEffect, useRef, useMemo } from 'preact/hooks';
import { useSearch } from '../hook/useSearch.js';

export const multiSelectStyleSheet = new CSSStyleSheet();
multiSelectStyleSheet.replaceSync(`
  .multi-select {
    display: flex;
    box-sizing: border-box;
    flex-direction: column;
    position: relative;
    width: 100%;
    user-select: none;
    --ms-bg: #ffffff;
    --text-color-dark: #212529;
    --text-color-light: #65727e;
    --border-color-light: #f1f3f5;
    --input-border: #dee2e6;
    --input-border-active: #c1c9d0;
    --option-background: #f3f4f7;
    --checkbox-border: #ced4da;
    --checkbox-background: #ffffff;
    --checkbox-active: #ffffff;
    --input-min-height: 2.8125rem;
    --options-height: 40dvh;
    --border-radius: 0.3125rem;
    --icon-size: 0.75rem;
    --icon-space: 1.875rem;
    --checkbox-size: 1rem;
    --checkbox-radius: 0.25rem;
    --checkbox-thickness: 0.125rem;
    --spacing-smaller: 0.1875rem;
    --spacing-small: 0.3125rem;
    --spacing-medium: 0.4375rem;
    --spacing-large: 0.75rem;
    --font-size-large: 0.875rem;
    --font-size-larger: 1rem;
    --line-height-larger: 1.25rem;
  }

  .multi-select .multi-select-header {
    display: flex;
    flex-wrap: wrap;
    box-sizing: border-box;
    align-items: center;
    border-radius: var(--border-radius);
    cursor: pointer;
    width: 100%;
    font-size: var(--font-size-larger);
    color: var(--text-color-dark);
    background-color: var(--ms-bg);
    border: 1px solid var(--input-border);
    padding: var(--spacing-medium) var(--spacing-large);
    padding-right: var(--icon-space);
    overflow: hidden;
    gap: var(--spacing-medium);
    min-height: var(--input-min-height);
  }

  .multi-select .multi-select-header::after {
    content: "";
    display: block;
    position: absolute;
    top: 50%;
    right: 1rem;
    transform: translateY(-50%);
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%23c9c9c9' viewBox='0 0 16 16'%3E%3Cpath d='M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z'/%3E%3C/svg%3E");
    height: var(--icon-size);
    width: var(--icon-size);
    background-repeat: no-repeat;
    background-size: contain;
  }

  .multi-select .multi-select-header:hover {
    border-color: var(--color-primary);
  }

  .multi-select .multi-select-header:hover::after {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%2316A085' viewBox='0 0 16 16'%3E%3Cpath d='M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z'/%3E%3C/svg%3E");
  }

  .multi-select .multi-select-header.multi-select-header-active {
    border-color: var(--input-border-active);
  }

  .multi-select .multi-select-header.multi-select-header-active::after {
    transform: translateY(-50%) rotate(180deg);
  }

  .multi-select .multi-select-header .multi-select-header-placeholder {
    color: var(--color-text);
  }

  .multi-select .multi-select-header .multi-select-header-option {
    display: inline-flex;
    align-items: center;
    background-color: var(--option-background);
    color: var(--text-color-dark);
    font-size: var(--font-size-large);
    padding: var(--spacing-smaller) var(--spacing-small);
    border-radius: var(--border-radius);
    gap: var(--spacing-smaller);
  }

  .multi-select .multi-select-header .multi-select-header-option-remove {
    background: none;
    border: none;
    cursor: pointer;
    padding: 0;
    line-height: 1;
    color: var(--text-color-dark);
    font-size: var(--font-size-larger);
  }

  .multi-select .multi-select-options {
    box-sizing: border-box;
    position: absolute;
    top: 100%;
    left: 0;
    right: 0;
    z-index: 999;
    margin-top: var(--spacing-small);
    padding: var(--spacing-small);
    background-color: var(--ms-bg);
    border-radius: var(--border-radius);
    box-shadow: 0 0.25rem 0.5rem rgba(0, 0, 0, 0.15);
    max-height: var(--options-height);
    overflow-y: auto;
    overflow-x: hidden;
  }

  .multi-select .multi-select-options::-webkit-scrollbar { width: 0.3125rem; }
  .multi-select .multi-select-options::-webkit-scrollbar-track { background: #e9e9ed; }
  .multi-select .multi-select-options::-webkit-scrollbar-thumb { background: #bebebe; }
  .multi-select .multi-select-options::-webkit-scrollbar-thumb:hover { background: #65727e; }

  .multi-select .multi-select-search-wrapper {
    width: 100%;
  }

  .multi-select .multi-select-search {
    padding: var(--spacing-medium) var(--spacing-large);
    border: 1px solid var(--input-border);
    border-radius: var(--border-radius);
    margin: 0.625rem;
    width: calc(100% - 1.25rem);
    outline: none;
    font-size: var(--font-size-larger);
    background-color: var(--ms-bg);
    color: var(--text-color-dark);
    box-sizing: border-box;
  }

  .multi-select .multi-select-search::placeholder { color: #65727e; }

  .multi-select .multi-select-option,
  .multi-select .multi-select-all {
    display: flex;
    flex-wrap: wrap;
    box-sizing: border-box;
    align-items: center;
    border-radius: var(--border-radius);
    cursor: pointer;
    width: 100%;
    font-size: var(--font-size-larger);
    color: var(--text-color-dark);
    padding: 0.5rem var(--spacing-large);
  }

  .multi-select .multi-select-option:hover,
  .multi-select .multi-select-option:active,
  .multi-select .multi-select-all:hover,
  .multi-select .multi-select-all:active {
    background-color: var(--option-background);
  }

  .multi-select .multi-select-all {
    border-bottom: 1px solid var(--border-color-light);
    border-radius: 0;
  }

  .multi-select .multi-select-option .multi-select-option-radio,
  .multi-select .multi-select-all .multi-select-option-radio {
    background: var(--checkbox-background);
    margin-right: var(--spacing-large);
    height: var(--checkbox-size);
    width: var(--checkbox-size);
    min-width: var(--checkbox-size);
    border: 1px solid var(--checkbox-border);
    border-radius: var(--checkbox-radius);
  }

  .multi-select .multi-select-option .multi-select-option-text,
  .multi-select .multi-select-all .multi-select-option-text {
    box-sizing: border-box;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    color: inherit;
    font-size: var(--font-size-larger);
    line-height: var(--line-height-larger);
    padding-bottom: 1px;
  }

  .multi-select .multi-select-option.multi-select-selected .multi-select-option-radio,
  .multi-select .multi-select-all.multi-select-selected .multi-select-option-radio {
    border-color: var(--color-primary);
    background-color: var(--color-primary);
  }

  .multi-select .multi-select-option.multi-select-selected .multi-select-option-radio::after,
  .multi-select .multi-select-all.multi-select-selected .multi-select-option-radio::after {
    content: "";
    display: block;
    width: calc(var(--checkbox-size) / 4);
    height: calc(var(--checkbox-size) / 2);
    border: solid var(--checkbox-active);
    border-width: 0 var(--checkbox-thickness) var(--checkbox-thickness) 0;
    transform: rotate(45deg) translate(50%, -25%);
  }

  .multi-select .multi-select-option.multi-select-selected .multi-select-option-text,
  .multi-select .multi-select-all.multi-select-selected .multi-select-option-text {
    color: var(--color-primary);
  }
`);

// Reference count to inject/remove the stylesheet only when the first/last instance mounts/unmounts
let mountCount = 0;

export function MultiSelectDropDown({ name, placeholder, children }) {
  const containerRef = useRef(null);
  const nativeSelectRef = useRef(null);
  const [isOpen, setIsOpen] = useState(false);
  const [selectedValues, setSelectedValues] = useState(null);

  // Derive a stable signature from children vnodes so rawOptions only recomputes
  // when the actual option data changes (vnodes are recreated on every parent render)
  const optionChildren = children.filter((child) => child && child.type === 'option');
  const optionSignature = optionChildren.map((n) => `${n.props.value}:${n.props.children}:${n.props.selected ? 1 : 0}`).join('|');

  const rawOptions = useMemo(
    () =>
      optionChildren.map((node) => ({
        value: String(node.props.value ?? ''),
        label: String(node.props.children ?? node.props.value ?? ''),
        defaultSelected: !!node.props.selected,
      })),
    [optionSignature]
  );

  useEffect(() => {
    if (mountCount === 0) {
      document.adoptedStyleSheets = [multiSelectStyleSheet, ...document.adoptedStyleSheets];
    }
    mountCount++;
    return () => {
      mountCount--;
      if (mountCount === 0) {
        document.adoptedStyleSheets = document.adoptedStyleSheets.filter((s) => s !== multiSelectStyleSheet);
      }
    };
  }, []);

  // pre-selections and prune values that no longer exist
  useEffect(() => {
    setSelectedValues((prev) => {
      if (prev === null) {
        return new Set(rawOptions.filter((o) => o.defaultSelected).map((o) => o.value));
      }
      const next = new Set(prev);
      rawOptions.forEach((o) => {
        if (o.defaultSelected) next.add(o.value);
      });
      for (const v of next) {
        if (!rawOptions.find((o) => o.value === v)) next.delete(v);
      }
      return next;
    });
  }, [rawOptions]);

  const items = useMemo(() => rawOptions.map(({ value, label }) => ({ value, label })), [rawOptions]);
  const { results: filteredItems, search } = useSearch(items, ['label']);

  // Sync native <select> so form.elements[name].selectedOptions works in pages
  useEffect(() => {
    if (!nativeSelectRef.current || selectedValues === null) return;
    Array.from(nativeSelectRef.current.options).forEach((opt) => {
      opt.selected = selectedValues.has(opt.value);
    });
  }, [selectedValues]);

  // Close on outside click
  useEffect(() => {
    if (!isOpen) return;

    function onOutside(e) {
      if (containerRef.current && !containerRef.current.contains(e.target)) setIsOpen(false);
    }

    document.addEventListener('click', onOutside);
    return () => document.removeEventListener('click', onOutside);
  }, [isOpen]);

  function toggleOpen(e) {
    e.stopPropagation();
    setIsOpen((o) => !o);
  }

  function handleOptionClick(value) {
    setSelectedValues((prev) => {
      const next = new Set(prev);
      next.has(value) ? next.delete(value) : next.add(value);
      return next;
    });
  }

  function removeTag(e, value) {
    e.stopPropagation();
    setSelectedValues((prev) => {
      const next = new Set(prev);
      next.delete(value);
      return next;
    });
  }

  const allFilteredSelected = filteredItems.length > 0 && filteredItems.every((o) => selectedValues?.has(o.value));

  function handleSelectAll(e) {
    e.stopPropagation();
    setSelectedValues((prev) => {
      const next = new Set(prev);
      allFilteredSelected ? filteredItems.forEach((o) => next.delete(o.value)) : filteredItems.forEach((o) => next.add(o.value));
      return next;
    });
  }

  const selectedItems = rawOptions.filter((o) => selectedValues?.has(o.value));

  return html`
    <div ref=${containerRef} class="multi-select" data-theme="light" aria-expanded=${isOpen}>
      <div
        class="multi-select-header${isOpen ? ' multi-select-header-active' : ''}"
        onClick=${toggleOpen}
        tabindex="0"
        onKeyDown=${(e) => e.key === 'Enter' && toggleOpen(e)}
      >
        ${
          selectedItems.length
            ? selectedItems.map(
                (item) => html`
                  <span key=${item.value} class="multi-select-header-option" data-value=${item.value}>
                    ${item.label}
                    <button
                      type="button"
                      class="multi-select-header-option-remove"
                      onClick=${(e) => removeTag(e, item.value)}
                      aria-label="Remove ${item.label}"
                    >
                      ×
                    </button>
                  </span>
                `
              )
            : html`<span class="multi-select-header-placeholder">${placeholder || 'Select item(s)'}</span>`
        }
      </div>

      ${
        isOpen &&
        html`
          <div class="multi-select-options">
            <div class="multi-select-search-wrapper">
              <input
                type="text"
                class="multi-select-search"
                placeholder="Search..."
                autofocus
                onInput=${(e) => search(e.target.value)}
                onClick=${(e) => e.stopPropagation()}
              />
            </div>
            <div class="multi-select-option multi-select-all${allFilteredSelected ? ' multi-select-selected' : ''}" onClick=${handleSelectAll}>
              <span class="multi-select-option-radio"></span>
              <span class="multi-select-option-text">Select All</span>
            </div>
            ${filteredItems.map(
              (item) => html`
                <div
                  key=${item.value}
                  class="multi-select-option${selectedValues?.has(item.value) ? ' multi-select-selected' : ''}"
                  data-value=${item.value}
                  onClick=${(e) => {
                    e.stopPropagation();
                    handleOptionClick(item.value);
                  }}
                >
                  <span class="multi-select-option-radio"></span>
                  <span class="multi-select-option-text">${item.label}</span>
                </div>
              `
            )}
          </div>
        `
      }

      <select ref=${nativeSelectRef} name=${name} class="pure-input-1" multiple style="display:none;" tabindex="-1" aria-hidden="true">
        ${rawOptions.map((o) => html`<option key=${o.value} value=${o.value}>${o.label}</option>`)}
      </select>
    </div>
  `;
}
