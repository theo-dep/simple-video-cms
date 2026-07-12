import { html } from 'htm/preact';
import { useState } from 'preact/hooks';

const shareIcon = html`
  <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-share-fill svg-button" viewBox="0 0 16 16">
    <path
      d="M11 2.5a2.5 2.5 0 1 1 .603 1.628l-6.718 3.12a2.5 2.5 0 0 1 0 1.504l6.718 3.12a2.5 2.5 0 1 1-.488.876l-6.718-3.12a2.5 2.5 0 1 1 0-3.256l6.718-3.12A2.5 2.5 0 0 1 11 2.5"
    />
  </svg>
`;

const copyIcon = html`
  <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-copy svg-button" viewBox="0 0 16 16">
    <path
      fill-rule="evenodd"
      d="M4 2a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2zm2-1a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1zM2 5a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1v-1h1v1a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h1v1z"
    />
  </svg>
`;

const copiedIcon = html`
  <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-check2-square svg-button" viewBox="0 0 16 16">
    <path
      d="M3 14.5A1.5 1.5 0 0 1 1.5 13V3A1.5 1.5 0 0 1 3 1.5h8a.5.5 0 0 1 0 1H3a.5.5 0 0 0-.5.5v10a.5.5 0 0 0 .5.5h10a.5.5 0 0 0 .5-.5V8a.5.5 0 0 1 1 0v5a1.5 1.5 0 0 1-1.5 1.5z"
    />
    <path d="m8.354 10.354 7-7a.5.5 0 0 0-.708-.708L8 9.293 5.354 6.646a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0" />
  </svg>
`;

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

  const icon = canShare ? shareIcon : copied ? copiedIcon : copyIcon;
  const label = canShare ? 'Share' : copied ? 'Copied' : 'Copy link';
  return html`
    <button class="share-button" onClick=${handleShare} aria-label=${label}>
      ${icon}
      <span>${label}</span>
    </button>
  `;
}
