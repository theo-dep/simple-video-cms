import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { Icon } from './Icon.js';

export function ShareButton() {
  const title = window.title;
  const url = location.href;
  const [copied, setCopied] = useState(false);
  const canShare = typeof navigator.share === 'function';

  async function handleShare() {
    const shareData = { title, url };

    if (canShare) {
      try {
        await navigator.share(shareData);
      } catch (err) {
        if (err.name !== 'AbortError') {
          console.error('Share failed:', err);
        }
      }
    } else {
      try {
        await navigator.clipboard.writeText(url);
        setCopied(true);
        setTimeout(() => setCopied(false), 2000);
      } catch (err) {
        console.error('Clipboard write failed:', err);
      }
    }
  }

  const icon = canShare
    ? html`<${Icon} name="share" fill class="svg-button" />`
    : copied
      ? html`<${Icon} name="check2-square" class="svg-button" />`
      : html`<${Icon} name="copy" class="svg-button" />`;

  const label = canShare ? 'Share' : copied ? 'Copied' : 'Copy link';
  return html`
    <button class="share-button" onClick=${handleShare} aria-label=${label}>
      ${icon}
      <span>${label}</span>
    </button>
  `;
}
