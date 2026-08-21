import { html } from 'htm/preact';
import { useState, useRef, useEffect } from 'preact/hooks';
import { Icon } from './Icon.js';

export function PasswordInput({ name, placeholder, autofocus, onKeydown }) {
  const [value, setValue] = useState('');
  const [visible, setVisible] = useState(false);
  const inputRef = useRef(null);

  useEffect(() => {
    if (autofocus) {
      setTimeout(() => inputRef.current?.focus(), 0);
    }
  }, [autofocus]);

  function handleKeydown(e) {
    if (e.key === 'Enter') {
      const form = inputRef.current?.closest('form');
      if (form) form.requestSubmit();
    }
    onKeydown?.(e);
  }

  return html`
    <div class="form">
      <input
        ref=${inputRef}
        class="input"
        type=${visible ? 'text' : 'password'}
        name=${name}
        placeholder=${placeholder}
        required
        value=${value}
        onInput=${(e) => setValue(e.target.value)}
        onKeydown=${handleKeydown}
      />
      <span class="field-icon" onClick=${() => setVisible(!visible)}>
        ${visible ? html`<${Icon} name="eye-slash" />` : html`<${Icon} name="eye" />`}
      </span>
    </div>
  `;
}
