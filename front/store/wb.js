import { Workbox, messageSW as postMessageToSW } from 'workbox-window';
import { signal } from '@preact/signals';
import { confirm } from '../component/ConfirmDialog.js';

export const wb = signal(null);
export const swReady = signal(false);
// Incremented when a service worker takes control of the page (mid-session update)
export const swControllerVersion = signal(0);

// Worker we can message. On a hard reload the page is not controlled by the
// service worker, but the active worker still answers postMessage.
let reachableSW = null;
let initPromise = null;
let updateAccepted = false;

// Old Safari (11.1, macOS 10.13) has broken service worker lifecycles:
// `ready` may never settle and workbox events may not fire. Never rely on a
// single signal: every wait races a timeout, and reachability is polled.
const SW_TIMEOUT_MS = 30000;
const REACHABLE_TIMEOUT_MS = 60000;

function withTimeout(promise, label) {
  return Promise.race([
    promise,
    new Promise((_resolve, reject) => {
      setTimeout(() => reject(new Error(`Service Worker ${label} timed out`)), SW_TIMEOUT_MS);
    }),
  ]);
}

function setReachableSW(sw) {
  if (!sw || reachableSW) return;
  reachableSW = sw;
  swReady.value = true;
}

// Poll navigator.serviceWorker.controller: on Safari 11.1 it is the only
// signal that reliably works once the worker activates (which can take a
// while: the precache runs during install).
function waitForReachableSW() {
  return new Promise((resolve) => {
    const started = Date.now();
    const poll = () => {
      const controller = navigator.serviceWorker?.controller;
      if (reachableSW || controller) {
        setReachableSW(controller);
        resolve();
      } else if (Date.now() - started > REACHABLE_TIMEOUT_MS) {
        resolve();
      } else {
        setTimeout(poll, 500);
      }
    };
    poll();
  });
}

export function initWorkbox() {
  if (!initPromise) {
    initPromise = doInitWorkbox();
  }
  return initPromise;
}

async function doInitWorkbox() {
  if (!('serviceWorker' in navigator)) {
    console.warn('Service Worker not supported - running without offline features');
    // App.js gates the first render on swReady: never stay stuck loading
    swReady.value = true;
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
    setReachableSW(event.sw);
  });

  workboxInstance.addEventListener('waiting', async (_event) => {
    console.log('New Service Worker waiting');
    updateAccepted = await confirm('A new version of the app is available. Reload now to update?', {
      confirmText: 'Reload',
      cancelText: 'Later',
    });
    if (updateAccepted) {
      workboxInstance.messageSkipWaiting();
    }
  });

  workboxInstance.addEventListener('controlling', (event) => {
    console.log('Service Worker now controls the page');
    setReachableSW(event.sw);
    swControllerVersion.value++;
    if (updateAccepted) {
      window.location.reload();
    }
  });

  // Register the Service Worker
  try {
    await withTimeout(workboxInstance.register(), 'registration');
  } catch (error) {
    // Never leave the app stuck on the loading screen
    console.warn('Service Worker registration failed:', error);
    swReady.value = true;
    return;
  }
  wb.value = workboxInstance;

  // Fast paths when the worker is already active; `ready` may hang forever
  // on Safari 11.1, so never await it directly - waitForReachableSW polls too.
  setReachableSW(navigator.serviceWorker.controller);
  navigator.serviceWorker.ready.then((registration) => setReachableSW(registration.active)).catch(() => {});
}

// Send a message to the Service Worker and wait for response
export async function messageSW(message) {
  // Wait for the full init: the app now renders before the SW is active
  // (precache can take a while), so the first message may arrive while
  // reachableSW is still null. initWorkbox is memoized: instant after init.
  await initWorkbox();
  if (!reachableSW) {
    // The worker may still be installing/activating: give it a real chance
    await waitForReachableSW();
  }
  if (!reachableSW) {
    throw new Error('Offline features unavailable on this browser');
  }
  const response = await withTimeout(postMessageToSW(reachableSW, message), 'message');
  return response;
}

// One wrapper per Service Worker message: the single place that knows the
// message shapes, so callers never build message objects themselves
export const swApi = {
  enableVideoCaching: () => messageSW({ type: 'enableVideoCaching' }),
  disableVideoCaching: () => messageSW({ type: 'disableVideoCaching' }),

  addVideoToOfflineCache: (id, title) => messageSW({ type: 'addVideoToOfflineCache', payload: { id, title } }),
  removeVideoFromOfflineCache: (id) => messageSW({ type: 'removeVideoFromOfflineCache', payload: { id } }),
  getAllCachedVideos: () => messageSW({ type: 'getAllCachedVideos' }),
  getAutoCachedVideos: () => messageSW({ type: 'getAutoCachedVideos' }),
  getStorageInfo: () => messageSW({ type: 'getStorageInfo' }),
  clearCachedVideos: () => messageSW({ type: 'clearCachedVideos' }),
  clearDownloadedVideos: () => messageSW({ type: 'clearDownloadedVideos' }),

  replayBookmarks: () => messageSW({ type: 'replayBookmarks' }),
};
