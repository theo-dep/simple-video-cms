import { html } from 'htm/preact';
import { useState } from 'preact/hooks';

export function Dropdown({ title, liElements }) {
  const [isOpen, setIsOpen] = useState(true);

  return html`
    <div class="dropdown">
      <button class="dropdown-toggle dropdown-toggle-${isOpen ? 'opened' : 'closed'}" onClick=${() => setIsOpen(!isOpen)}>
        ${title}
        <svg
          class="dropdown-arrow dropdown-arrow-${isOpen ? 'opened' : 'closed'}"
          xmlns="http://www.w3.org/2000/svg"
          fill="currentColor"
          viewBox="0 0 16 16"
        >
          <path d="M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z" />
        </svg>
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
