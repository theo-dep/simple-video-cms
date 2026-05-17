import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { XIcon } from '../svg/XIcon.js';

export function Alert({ message }) {
  const [visible, setVisible] = useState(true);

  useEffect(() => {
    if (message) setVisible(true);
  }, [message]);

  if (!message || !visible) return null;

  return html`
    <div class="pure-form pure-form-aligned">
      <div class="alert pure-control-group" onClick=${() => setVisible(false)}>
        <svg class="close-button">
          <title>Close</title>
          <${XIcon} />
        </svg>
        <p>${message}</p>
      </div>
    </div>
  `;
}
