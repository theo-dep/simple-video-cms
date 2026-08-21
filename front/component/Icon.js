import { html } from 'htm/preact';

export function Icon({ name, class: className = '', fill = false, ...props }) {
  const iconClass = fill ? `bi bi-${name}-fill` : `bi bi-${name}`;
  return html` <i class="${iconClass} ${className}" ...${props} /> `;
}
