import { html } from 'htm/preact';
import { Icon } from './Icon.js';

export function ToggleIconButton({
  isActive,
  onClick,
  activeIcon,
  inactiveIcon,
  activeTitle,
  inactiveTitle,
  loading,
  error,
  location,
  className = '',
}) {
  const icon = isActive ? html`<${Icon} name=${activeIcon} />` : html`<${Icon} name=${inactiveIcon} />`;
  const title = error || (isActive ? activeTitle : inactiveTitle);

  return html`
    <div
      class="toggle-button ${isActive ? 'is-active' : ''} ${loading ? 'is-loading' : ''} ${error ? 'has-error' : ''} ${className}"
      location=${location}
      onClick=${onClick}
      title=${title}
    >
      ${loading ? html`<${Icon} name="three-dots" class="spin" />` : icon} ${error && html`<span class="toggle-button-tooltip">${error}</span>`}
    </div>
  `;
}
