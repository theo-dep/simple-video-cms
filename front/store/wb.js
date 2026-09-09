import { Workbox, messageSW as postMessageToSW } from 'workbox-window';
import { signal } from '@preact/signals';

export const wb = signal(null);
export const swReady = signal(false);

// Worker we can message. On a hard reload the page is not controlled by the
// service worker, but the active worker still answers postMessage.
let reachableSW = null;
let initPromise = null;

export function initWorkbox() {
  if (!initPromise) {
    initPromise = doInitWorkbox();
  }
  return initPromise;
}

async function doInitWorkbox() {
  if (!('serviceWorker' in navigator)) {
    console.warn('Service Worker not supported');
    return;
  }

  const workboxInstance = new Workbox('/sw.js');

  // Listen to important events
  workboxInstance.addEventListener('installed', (event) => {
    if (event.isUpdate) {
      console.log('Service Worker newly installed');
    } else {
      console.log('Service Worker installed for the first time');
    }
  });

  workboxInstance.addEventListener('activated', (event) => {
    if (event.isUpdate) {
      console.log('Service Worker newly activated');
    } else {
      console.log('Service Worker activated for the first time');
    }
    reachableSW = event.sw;
    swReady.value = true;
  });

  workboxInstance.addEventListener('waiting', (_event) => {
    console.log('New Service Worker waiting');
  });

  workboxInstance.addEventListener('controlling', (event) => {
    console.log('Service Worker now controls the page');
    reachableSW = event.sw;
    swReady.value = true;
  });

  // Register the Service Worker
  await workboxInstance.register();
  wb.value = workboxInstance;

  if (!reachableSW) {
    // `controlling` is a promise, not a boolean: resolve the worker ourselves.
    reachableSW = navigator.serviceWorker.controller ?? (await navigator.serviceWorker.ready).active ?? null;
    if (reachableSW) {
      swReady.value = true;
    }
  }
}

// Send a message to the Service Worker and wait for response
export async function messageSW(message) {
  if (!wb.value) {
    await initWorkbox();
  }
  if (!reachableSW) {
    throw new Error('No reachable service worker');
  }
  return postMessageToSW(reachableSW, message);
}
