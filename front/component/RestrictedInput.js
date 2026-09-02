import { html } from 'htm/preact';
import { useState, useRef, useEffect } from 'preact/hooks';
import { signal } from '@preact/signals';
import { Icon } from './Icon.js';

const tooltipField = signal(null);

export function RestrictedInput({
  name,
  placeholder,
  type = 'text',
  value,
  onInput,
  required = false,
  autofocus = false,
  tooltip = '',
  ref,
  ...props
}) {
  const [showTooltip, setShowTooltip] = useState(false);
  const tooltipRef = useRef(null);
  const inputRef = ref || useRef(null);

  useEffect(() => {
    function handleClickOutside(e) {
      if (tooltipRef.current && !tooltipRef.current.contains(e.target)) {
        setShowTooltip(false);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  function toggleTooltip(e) {
    e.preventDefault();
    e.stopPropagation();
    setShowTooltip(!showTooltip);
  }

  return html`
    <div class="form-restricted-input">
      <input
        ref=${inputRef}
        class="input"
        type=${type}
        name=${name}
        placeholder=${placeholder}
        value=${value}
        onInput=${onInput}
        required=${required}
        autofocus=${autofocus}
        onClick=${() => (tooltipField.value = inputRef.current)}
        ...${props}
      />
      ${
        tooltip &&
        tooltipField.value &&
        tooltipField.value == inputRef.current &&
        html`
          <span class="field-icon" onClick=${toggleTooltip} ref=${tooltipRef}>
            <${Icon} name="question-circle" />
            ${showTooltip && html` <span class="input-tooltip"> ${tooltip} </span> `}
          </span>
        `
      }
    </div>
  `;
}
