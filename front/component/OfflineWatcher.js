import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { apiOffline } from '../store/offline.js';
import { swApi } from '../store/wb.js';

// Replay bookmark requests the SW queued while offline
async function replayQueuedBookmarks() {
  try {
    await swApi.replayBookmarks();
  } catch {
    // no reachable service worker: nothing to replay
  }
}

export function OfflineWatcher() {
  useEffect(() => {
    const handleApiOnline = () => {
      apiOffline.value = false;
      dispatchEvent(new CustomEvent('retry-fetches'));
      void replayQueuedBookmarks();
    };

    const handleApiOffline = () => {
      apiOffline.value = true;
    };

    const handleRejection = (_event) => {
      if (!navigator.onLine) {
        apiOffline.value = true;
      }
    };

    void replayQueuedBookmarks();

    addEventListener('api-offline', handleApiOffline);
    addEventListener('unhandledrejection', handleRejection);
    addEventListener('online', handleApiOnline);
    return () => {
      removeEventListener('api-offline', handleApiOffline);
      removeEventListener('unhandledrejection', handleRejection);
      removeEventListener('online', handleApiOnline);
    };
  }, []);

  return !apiOffline.value
    ? null
    : html`<div class="offline-banner">
        <span>You're offline. Some content may be unavailable.</span>
      </div>`;
}
