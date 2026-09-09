import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { refreshStorageInfo, formatBytes, cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';
import { Icon } from './Icon.js';

export function StorageIndicator() {
  useEffect(() => {
    if (swReady.value) {
      refreshStorageInfo();
    }
  }, [swReady.value]);

  const info = cache.storageInfo.value;

  if (!info) {
    return html`
      <div class="storage-indicator" title="Loading storage info..."><${Icon} name="hdd" /> <${Icon} name="three-dots" class="spin" /></div>
    `;
  }

  const percentageUsed = Math.round(info.percentageUsed || 0);
  const used = formatBytes(info.usage || 0);
  const available = formatBytes(info.available || 0);

  return html`
    <div class="storage-indicator" title=${`${used} used, ${available} available (${percentageUsed}% used)`}>
      <${Icon} name="hdd" />
      <span class="storage-used">${used}</span>
      <span class="storage-bar" role="progressbar" aria-valuenow=${percentageUsed} aria-valuemin="0" aria-valuemax="100">
        <span class="storage-bar-fill" style=${`width: ${percentageUsed}%`}></span>
      </span>
      <span class="storage-available">${available} free</span>
    </div>
  `;
}
