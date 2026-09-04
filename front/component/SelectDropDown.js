import { html } from 'htm/preact';
import { useState, useEffect, useRef, useMemo, useImperativeHandle } from 'preact/hooks';
import { forwardRef } from 'preact/compat';
import { useSearch } from '../hook/useSearch.js';
import { Icon } from './Icon.js';

function SelectedTags({ items, onRemove }) {
  return items.map(
    (item) => html`
      <span key=${item.value} class="select-dropdown-tag" data-value=${item.value}>
        ${item.label}
        <button type="button" class="select-dropdown-tag-remove" onClick=${(e) => onRemove(e, item.value)} aria-label="Remove ${item.label}">
          <${Icon} name="x" />
        </button>
      </span>
    `
  );
}

function SelectAllRow({ checked, onClick, deletable, editable }) {
  return html`
    <div
      class="select-dropdown-option select-dropdown-option-all${checked ? ' select-dropdown-option-selected' : ''} ${editable || deletable ? 'select-dropdown-option-all-shift' : ''}"
      onClick=${onClick}
    >
      <span class="select-dropdown-option-checkbox"></span>
      <span class="select-dropdown-option-text">Select All</span>
    </div>
  `;
}

function OptionRow({ item, selected, onSelect, deletable, onDelete, editable, onEdit }) {
  const [isEditing, setIsEditing] = useState(false);
  const [editValue, setEditValue] = useState(item.label);
  const inputRef = useRef(null);
  const optionRef = useRef(null);

  function startEditing(e) {
    e.stopPropagation();
    setEditValue(item.label);
    setIsEditing(true);
  }

  function stopEditing(e) {
    e.stopPropagation();
    setIsEditing(false);
  }

  function handleEditKeyDown(e) {
    if (e.key === 'Enter') {
      handleSave(e);
    }
    if (e.key === 'Escape') {
      e.stopPropagation();
      setIsEditing(false);
    }
  }

  function handleSave(e) {
    e.stopPropagation();
    onEdit?.(item.value, editValue.trim());
    setIsEditing(false);
  }

  function handleDelete(e) {
    onDelete?.(e, item.value);
  }

  function handleSelect(e) {
    e.stopPropagation();
    onSelect(item.value);
  }

  useEffect(() => {
    if (isEditing && inputRef.current) {
      inputRef.current.focus();
      inputRef.current.select();
    }
  }, [isEditing]);

  useEffect(() => {
    if (!isEditing) return;
    function handleClickOutside(e) {
      if (optionRef.current && !optionRef.current.contains(e.target)) {
        setIsEditing(false);
      }
    }
    document.addEventListener('click', handleClickOutside);
    return () => document.removeEventListener('click', handleClickOutside);
  }, [isEditing]);

  return html`
    <div
      ref=${optionRef}
      class="select-dropdown-option${selected ? ' select-dropdown-option-selected' : ''}"
      data-value=${item.value}
      onClick=${handleSelect}
    >
      <span class="select-dropdown-option-checkbox"></span>
      ${
        isEditing
          ? html`
              <input
                ref=${inputRef}
                type="text"
                class="select-dropdown-option-input"
                value=${editValue}
                onInput=${(e) => setEditValue(e.target.value)}
                onKeyDown=${handleEditKeyDown}
                onClick=${(e) => e.stopPropagation()}
              />
            `
          : html`<span class="select-dropdown-option-text">${item.label}</span>`
      }
      ${
        editable &&
        html`
          ${
            isEditing
              ? html`
                  <button
                    type="button"
                    class="select-dropdown-option-button select-dropdown-option-save"
                    onClick=${handleSave}
                    aria-label="Save ${item.label}"
                  >
                    <${Icon} name="check-square" />
                  </button>
                  <button
                    type="button"
                    class="select-dropdown-option-button select-dropdown-option-cancel"
                    onClick=${stopEditing}
                    aria-label="Cancel"
                  >
                    <${Icon} name="x-square" />
                  </button>
                `
              : html`
                  <button
                    type="button"
                    class="select-dropdown-option-button select-dropdown-option-edit"
                    onClick=${startEditing}
                    aria-label="Edit ${item.label}"
                  >
                    <${Icon} name="pencil-square" />
                  </button>
                `
          }
        `
      }
      ${
        deletable &&
        !isEditing &&
        html`
          <button
            type="button"
            class="select-dropdown-option-button select-dropdown-option-delete"
            onClick=${handleDelete}
            aria-label="Delete ${item.label}"
          >
            <${Icon} name="trash" fill />
          </button>
        `
      }
    </div>
  `;
}

function SearchInput({ editable, value, onInput, onAdd }) {
  if (!editable) {
    return html`
      <div class="select-dropdown-search-wrapper">
        <input
          type="text"
          class="select-dropdown-search"
          placeholder="Search..."
          autofocus
          onInput=${onInput}
          onClick=${(e) => e.stopPropagation()}
        />
      </div>
    `;
  }

  return html`
    <div class="select-dropdown-search-wrapper select-dropdown-search-wrapper-editable">
      <input
        type="text"
        class="select-dropdown-search"
        placeholder="Search or add..."
        value=${value}
        autofocus
        onInput=${onInput}
        onKeyDown=${(e) => {
          if (e.key === 'Enter') {
            e.preventDefault();
            onAdd(e);
          }
        }}
        onClick=${(e) => e.stopPropagation()}
      />
      <button type="button" class="select-dropdown-add-btn" onClick=${onAdd} aria-label="Add option"><${Icon} name="plus-square" fill /></button>
    </div>
  `;
}

// multiple: allow several selections + Select All row
// editable: search box can add new options + delete existing ones, single-select auto-closes on pick
const SelectDropDown = forwardRef(
  ({ name, placeholder, children, multiple = false, editable = false, onChange, onAddedOption, onDeletedOption, onEditOption }, ref) => {
    const singleSelect = !multiple;
    const autoClose = editable && singleSelect;

    const containerRef = useRef(null);
    const nativeSelectRef = useRef(null);
    const pendingAutoSelectRef = useRef(new Set()); // values typed by user, awaiting parent to add the option
    const [isOpen, setIsOpen] = useState(false);
    const [selectedValues, setSelectedValues] = useState(null);
    const [searchText, setSearchText] = useState('');

    function clear() {
      setSelectedValues(new Set());
    }

    useImperativeHandle(ref, () => ({ clear }));

    // stable signature so rawOptions only recomputes when option data actually changes
    const optionChildren = [children].flat().filter((child) => child && child.type === 'option');
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

    // pre-selections, pending auto-select, prune stale values
    useEffect(() => {
      setSelectedValues((prev) => {
        if (prev === null) {
          if (singleSelect) {
            const next = new Set();
            const defaults = rawOptions.filter((o) => o.defaultSelected);
            if (defaults.length) next.add(defaults[defaults.length - 1].value);
            return next;
          }
          return new Set(rawOptions.filter((o) => o.defaultSelected).map((o) => o.value));
        }
        const next = new Set(prev);
        if (!singleSelect) {
          rawOptions.forEach((o) => {
            if (o.defaultSelected) next.add(o.value);
          });
        }
        for (const v of pendingAutoSelectRef.current) {
          if (rawOptions.find((o) => o.value === v)) {
            if (singleSelect) next.clear();
            next.add(v);
            pendingAutoSelectRef.current.delete(v);
          }
        }
        for (const v of next) {
          if (!rawOptions.find((o) => o.value === v)) next.delete(v);
        }
        return next;
      });
    }, [rawOptions]);

    const items = useMemo(() => rawOptions.map(({ value, label }) => ({ value, label })), [rawOptions]);
    const { results: filteredItems, search } = useSearch(items, ['label']);

    // sync native <select> so form.elements[name].selectedOptions works in pages
    useEffect(() => {
      if (!nativeSelectRef.current || selectedValues === null) return;
      Array.from(nativeSelectRef.current.options).forEach((opt) => {
        opt.selected = singleSelect && opt.value === '' ? selectedValues.size === 0 : selectedValues.has(opt.value);
      });
      onChange?.([...selectedValues]);
    }, [selectedValues]);

    useEffect(() => {
      if (!isOpen) return;
      function onOutside(e) {
        if (containerRef.current && !containerRef.current.contains(e.target)) setIsOpen(false);
      }
      document.addEventListener('click', onOutside);
      return () => document.removeEventListener('click', onOutside);
    }, [isOpen]);

    // only one dropdown open at a time
    useEffect(() => {
      function onOtherOpen(e) {
        if (e.detail !== containerRef.current) setIsOpen(false);
      }
      document.addEventListener('select-dropdown-open', onOtherOpen);
      return () => document.removeEventListener('select-dropdown-open', onOtherOpen);
    }, []);

    function toggleOpen(e) {
      e.stopPropagation();
      setIsOpen((o) => {
        if (!o) document.dispatchEvent(new CustomEvent('select-dropdown-open', { detail: containerRef.current }));
        return !o;
      });
    }

    function closeSearch() {
      if (autoClose) setIsOpen(false);
      setSearchText('');
      search('');
    }

    function selectValue(value) {
      setSelectedValues((prev) => {
        if (singleSelect) return new Set([value]);
        const next = new Set(prev);
        next.has(value) ? next.delete(value) : next.add(value);
        return next;
      });
      if (autoClose) closeSearch();
    }

    function removeValue(e, value) {
      e.stopPropagation();
      setSelectedValues((prev) => {
        const next = new Set(prev);
        next.delete(value);
        return next;
      });
    }

    function deleteOption(e, value) {
      e.stopPropagation();
      onDeletedOption?.(value);
      setSelectedValues((prev) => {
        const next = new Set(prev);
        next.delete(value);
        return next;
      });
    }

    const allFilteredSelected = multiple && filteredItems.length > 0 && filteredItems.every((o) => selectedValues?.has(o.value));

    function selectAll(e) {
      e.stopPropagation();
      setSelectedValues((prev) => {
        const next = new Set(prev);
        allFilteredSelected ? filteredItems.forEach((o) => next.delete(o.value)) : filteredItems.forEach((o) => next.add(o.value));
        return next;
      });
    }

    async function addOption(e) {
      e?.stopPropagation();
      const trimmed = searchText.trim();
      if (!trimmed) return;
      const lower = trimmed.toLowerCase();
      const match = rawOptions.find((o) => o.label.toLowerCase() === lower || o.value.toLowerCase() === lower);
      if (match) {
        selectValue(match.value);
      } else {
        const newValue = await onAddedOption?.(trimmed);
        if (newValue !== null) {
          pendingAutoSelectRef.current.add(String(newValue));
        }
        closeSearch();
      }
    }

    const selectedItems = rawOptions.filter((o) => selectedValues?.has(o.value));

    return html`
      <div ref=${containerRef} class="select-dropdown" data-theme="light" aria-expanded=${isOpen}>
        <div
          class="select-dropdown-header${isOpen ? ' select-dropdown-header-active' : ''}"
          onClick=${toggleOpen}
          tabindex="0"
          onKeyDown=${(e) => e.key === 'Enter' && toggleOpen(e)}
        >
          ${
            selectedItems.length
              ? html`<${SelectedTags} items=${selectedItems} onRemove=${removeValue} />`
              : html`<span class="select-dropdown-placeholder">${placeholder}</span>`
          }
          <${Icon} name="chevron-down" class="select-dropdown-arrow" />
        </div>
        ${
          isOpen &&
          html`
            <div class="select-dropdown-options">
              <${SearchInput}
                editable=${editable}
                value=${searchText}
                onInput=${(e) => {
                  setSearchText(e.target.value);
                  search(e.target.value);
                }}
                onAdd=${addOption}
              />
              ${multiple && html`<${SelectAllRow} checked=${allFilteredSelected} onClick=${selectAll} editable=${editable} deletable=${editable} />`}
              ${filteredItems.map(
                (item) =>
                  html`<${OptionRow}
                    key=${item.value}
                    item=${item}
                    selected=${!!selectedValues?.has(item.value)}
                    onSelect=${selectValue}
                    editable=${editable}
                    onEdit=${onEditOption}
                    deletable=${editable}
                    onDelete=${deleteOption}
                  />`
              )}
            </div>
          `
        }
        <select ref=${nativeSelectRef} name=${name} class="input" multiple=${multiple} style="display:none;" tabindex="-1" aria-hidden="true">
          ${singleSelect && html`<option value=""></option>`}
          ${rawOptions.map((o) => html`<option key=${o.value} value=${o.value}>${o.label}</option>`)}
        </select>
      </div>
    `;
  }
);

export const MultiSelectDropDown = forwardRef(({ name, placeholder, onChange, children }, ref) => {
  return html`<${SelectDropDown} ref=${ref} name=${name} placeholder=${placeholder || 'Select item(s)'} multiple onChange=${onChange}
    >${children}<//
  >`;
});

export const SingleSelectEditableDropDown = forwardRef(({ name, placeholder, children, onAddedOption, onDeletedOption, onEditOption }, ref) => {
  return html`<${SelectDropDown}
    ref=${ref}
    name=${name}
    placeholder=${placeholder || 'Select item'}
    editable
    onAddedOption=${onAddedOption}
    onDeletedOption=${onDeletedOption}
    onEditOption=${onEditOption}
    >${children}<//
  >`;
});

export const MultiSelectEditableDropDown = forwardRef(({ name, placeholder, children, onAddedOption, onDeletedOption, onEditOption }, ref) => {
  return html`<${SelectDropDown}
    ref=${ref}
    name=${name}
    placeholder=${placeholder || 'Select item(s)'}
    multiple
    editable
    onAddedOption=${onAddedOption}
    onDeletedOption=${onDeletedOption}
    onEditOption=${onEditOption}
    >${children}<//
  >`;
});
