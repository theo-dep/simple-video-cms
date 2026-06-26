import { html } from 'htm/preact';
import { signal } from '@preact/signals';
import { useEffect } from 'preact/hooks';

const state = signal(null);

export function confirm(message, options = {}) {
  if (state.value) state.value.resolve(false);

  return new Promise((resolve) => {
    state.value = {
      message,
      confirmText: options.confirmText ?? 'OK',
      cancelText: options.cancelText ?? 'Cancel',
      resolve,
    };
  });
}

function close(result) {
  const current = state.value;
  if (!current) return;
  state.value = null;
  current.resolve(result);
}

export function ConfirmDialog() {
  const current = state.value;

  useEffect(() => {
    if (!current) return;

    const previous = document.activeElement;
    document.getElementById('ok-btn')?.focus();

    const onKey = (e) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        close(false);
      } else if (e.key === 'Enter') {
        e.preventDefault();
        close(true);
      }
    };
    document.addEventListener('keydown', onKey);

    history.pushState({ confirmDialog: true }, '');
    const onPopState = () => close(false);
    window.addEventListener('popstate', onPopState);

    return () => {
      previous?.focus();
      document.removeEventListener('keydown', onKey);
      window.removeEventListener('popstate', onPopState);
      if (history.state?.confirmDialog) history.back();
    };
  }, [current]);

  if (!current) return null;

  return html`
    <div class="confirm-overlay" onClick=${() => close(false)}>
      <div class="confirm-modal" role="dialog" aria-modal="true" aria-describedby="confirm-msg" onClick=${(e) => e.stopPropagation()}>
        <p id="confirm-msg" class="confirm-message">${current.message}</p>
        <div class="confirm-actions">
          <button id="ok-btn" type="button" class="button confirm-ok" onClick=${() => close(true)}>${current.confirmText}</button>
          <button id="cancel-btn" type="button" class="button confirm-cancel" onClick=${() => close(false)}>${current.cancelText}</button>
        </div>
      </div>
    </div>
  `;
}
