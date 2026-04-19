import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { XLargeIcon } from '../svg/XLargeIcon.js';

export function Drawer({ label, items }) {
  const [open, setOpen] = useState(false);

  useEffect(() => {
    if (!open) return;
    const onPop = () => setOpen(false);
    window.addEventListener('popstate', onPop, { once: true });
    return () => window.removeEventListener('popstate', onPop);
  }, [open]);

  function openDrawer() {
    setOpen(true);
    history.pushState(null, '');
  }

  if (!items?.length) return null;

  const firstIdx = items.findIndex((i) => i.elements.length > 0);

  return html`
    <span class="drawer-button open-drawer" onClick=${openDrawer}>${label}</span>

    ${open &&
    html`
      <div class="drawer-overlay" onClick=${() => setOpen(false)}>
        <div class="drawer" aria-modal="true" role="dialog" onClick=${(e) => e.stopPropagation()}>
          ${items.map(
            (i, idx) =>
              !!i.elements?.length &&
              html`
                <div class="drawer-title">
                  <h1>${i.label}</h1>

                  ${idx === firstIdx &&
                  html`
                    <span class="drawer-button drawer-close" onClick=${() => setOpen(false)}>
                      <svg class="svg-button">
                        <title>Close</title>
                        <${XLargeIcon} />
                      </svg>
                    </span>
                  `}
                </div>
                <div class="elements-grid">${i.elements.map((e) => html`<div class="element-cell" key=${e}><p>${e}</p></div>`)}</div>
              `
          )}
        </div>
      </div>
    `}
  `;
}
