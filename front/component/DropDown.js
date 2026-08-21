import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { Icon } from './Icon.js';

export function Dropdown({ title, open = true, liElements }) {
  const [isOpen, setIsOpen] = useState(open);

  function onClick(e) {
    e.preventDefault();
    setIsOpen(!isOpen);
  }

  return html`
    <div class="dropdown">
      <button class="dropdown-toggle dropdown-toggle-${isOpen ? 'opened' : 'closed'}" onClick=${onClick}>
        ${title}
        <${Icon} name="chevron-down" class="dropdown-arrow dropdown-arrow-${isOpen ? 'opened' : 'closed'}" />
      </button>
      ${
        isOpen &&
        html`
          <div class="dropdown-menu-content">
            <ul class="dropdown-menu">
              ${liElements}
            </ul>
          </div>
        `
      }
    </div>
  `;
}
