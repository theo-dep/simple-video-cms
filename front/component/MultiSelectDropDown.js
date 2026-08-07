import { html } from 'htm/preact';
import { useState, useEffect, useRef, useMemo } from 'preact/hooks';
import { useSearch } from '../hook/useSearch.js';
import { useStyleSheet } from '../hook/useStyleSheet.js';
import { ArrowDownIcon } from '../svg/ArrowDownIcon.js';
import { XIcon } from '../svg/XIcon.js';
import { Loader } from './Loader.js';

import MultiSelectDropDownStyleSheet from './MultiSelectDropDown.css' with { type: 'css' };

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

  const isAdoptedStyleSheets = useStyleSheet(MultiSelectDropDownStyleSheet);
  if (!isAdoptedStyleSheets) {
    return html`<${Loader} />`;
  }

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
                      <${XIcon} />
                    </button>
                  </span>
                `
              )
            : html`<span class="multi-select-header-placeholder">${placeholder || 'Select item(s)'}</span>`
        }
        <${ArrowDownIcon} class="multi-select-arrow" />
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
