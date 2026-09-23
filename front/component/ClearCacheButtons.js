import { html } from 'htm/preact';
import { clearCachedVideos, clearDownloadedVideos } from '../store/cache.js';
import { confirm } from './ConfirmDialog.js';
import { Icon } from './Icon.js';

export function ClearCacheButton() {
  async function clear() {
    const message = 'Clear all cached videos?';
    if (!(await confirm(message))) return;

    await clearCachedVideos();
  }

  return html` <button type="button" class="button cache-button" onClick=${() => clear()}><${Icon} name="trash" /> Clear cache</button> `;
}

export function ClearDownloadButton() {
  async function clear() {
    const message = 'Remove all downloaded videos?';
    if (!(await confirm(message))) return;

    await clearDownloadedVideos();
  }

  return html` <button type="button" class="button cache-button" onClick=${() => clear()}><${Icon} name="trash" /> Clear downloads</button> `;
}

export function ClearCacheButtons() {
  return html`
    <div class="cache-actions">
      <${ClearCacheButton} />
      <${ClearDownloadButton} />
    </div>
  `;
}
