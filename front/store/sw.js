import { signal } from '@preact/signals';

export const swReady = signal(false);

export function postToServiceWorker(message) {
  if (!('serviceWorker' in navigator)) return;

  navigator.serviceWorker.getRegistration().then((registration) => {
    const sw = registration?.installing || registration?.waiting || registration?.active;
    sw?.postMessage(message);
  });
}
