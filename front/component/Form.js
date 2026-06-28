import { html } from 'htm/preact';
import { useState, useEffect, useLayoutEffect, useRef } from 'preact/hooks';
import MultiSelect from 'multi-select-dropdown-js/MultiSelect.js';
import { InfoContent } from './InfoContent.js';
import { SubmitButton } from './SubmitButton.js';
import { Alert } from './Alert.js';

import MultiSelectStyleSheet from 'multi-select-dropdown-js/MultiSelect.css' with { type: 'css' };

const additionalMultiSelectStyle = new CSSStyleSheet();
additionalMultiSelectStyle.replaceSync(`
.multi-select .multi-select-header:hover {
  border-color: #16a085;
}

.multi-select .multi-select-header::after {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%23c9c9c9' viewBox='0 0 16 16'%3E%3Cpath d='M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z'/%3E%3C/svg%3E");
}

.multi-select .multi-select-header:hover::after {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='%2316A085' viewBox='0 0 16 16'%3E%3Cpath d='M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z'/%3E%3C/svg%3E");
}

.multi-select .multi-select-header .multi-select-header-placeholder {
  color: #c9c9c9;
}

.multi-select .multi-select-options .multi-select-option.multi-select-selected .multi-select-option-radio,
.multi-select .multi-select-options .multi-select-all.multi-select-selected .multi-select-option-radio {
  border-color: #16a085;
  background-color: #16a085;
}

.multi-select .multi-select-options .multi-select-option.multi-select-selected .multi-select-option-text,
.multi-select .multi-select-options .multi-select-all.multi-select-selected .multi-select-option-text {
  color: #16a085;
}
`);

export function useMultiSelect(deps = []) {
  const ref = useRef(null);

  useLayoutEffect(() => {
    if (ref.current && !ref.current._multiSelect) {
      new MultiSelect(ref.current, { theme: 'light' });
    }
  }, deps);

  return ref;
}

export function FormContent({ title, buttonTitle, onSubmitAction, children }) {
  const [loading, setLoading] = useState(false);
  const [alert, setAlert] = useState('');

  useEffect(() => {
    document.adoptedStyleSheets = [MultiSelectStyleSheet, additionalMultiSelectStyle, ...document.adoptedStyleSheets];
    return () => {
      document.adoptedStyleSheets = document.adoptedStyleSheets.filter((s) => {
        return s !== MultiSelectStyleSheet || s !== additionalMultiSelectStyle;
      });
    };
  }, []);

  async function onSubmit(e) {
    e.preventDefault();

    setLoading(true);
    setAlert('');
    try {
      await onSubmitAction(e);
    } catch (err) {
      setAlert(err.message || 'Unknown error');
    } finally {
      setLoading(false);
    }
  }

  return html`
    <h3>${title}</h3>

    <form class="pure-form pure-form-aligned" onSubmit=${onSubmit}>
      <fieldset>
        <${Alert} message=${alert} />
        ${children}
        <div class="pure-control-group">
          <${SubmitButton} label="${buttonTitle}" loading=${loading} id="submit-button" />
        </div>
      </fieldset>
    </form>
  `;
}

export function Form({ title, buttonTitle, onSubmitAction, children }) {
  return html`
    <${InfoContent}>
      <${FormContent} title=${title} buttonTitle=${buttonTitle} onSubmitAction=${onSubmitAction}> ${children} <//>
    <//>
  `;
}
