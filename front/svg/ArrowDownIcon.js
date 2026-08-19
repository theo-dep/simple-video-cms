import { html } from 'htm/preact';

export function ArrowDownIcon({ class: className }) {
  return html`
    <svg class=${className} xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
      <path d="M8 13.1l-8-8 2.1-2.2 5.9 5.9 5.9-5.9 2.1 2.2z" />
    </svg>
  `;
}
