import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { apiOffline } from '../store/offline.js';

export function OfflineWatcher() {
  useEffect(() => {
    const handleApiOnline = () => {
      apiOffline.value = false;
      dispatchEvent(new CustomEvent('retry-fetches'));
    };

    const handleApiOffline = () => {
      apiOffline.value = true;
    };

    const handleRejection = (_event) => {
      if (!navigator.onLine) {
        apiOffline.value = true;
      }
    };

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
