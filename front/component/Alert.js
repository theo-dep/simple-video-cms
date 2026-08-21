import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { Icon } from './Icon.js';

export function Alert({ message }) {
  const [visible, setVisible] = useState(true);

  useEffect(() => {
    if (message) setVisible(true);
  }, [message]);

  if (!message || !visible) return null;

  return html`
    <div class="form">
      <div class="alert form-control-group" onClick=${() => setVisible(false)}>
        <span class="close-button" aria-label="Close">
          <${Icon} name="x" />
        </span>
        <p>${message}</p>
      </div>
    </div>
  `;
}
