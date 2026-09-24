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
    reachableSW = event.sw;
    swReady.value = true;
    swControllerVersion.value++;
    if (updateAccepted) {
      window.location.reload();
    }
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

// One wrapper per Service Worker message: the single place that knows the
// message shapes, so callers never build message objects themselves
export const swApi = {
  enableVideoCaching: () => messageSW({ type: 'enableVideoCaching' }),
  disableVideoCaching: () => messageSW({ type: 'disableVideoCaching' }),

  setVideoSession: (videoId, session) => messageSW({ type: 'setVideoSession', payload: { videoId, session } }),
  clearVideoSession: (videoId) => messageSW({ type: 'clearVideoSession', payload: { videoId } }),

  addVideoToOfflineCache: (id, title) => messageSW({ type: 'addVideoToOfflineCache', payload: { id, title } }),
  removeVideoFromOfflineCache: (id) => messageSW({ type: 'removeVideoFromOfflineCache', payload: { id } }),
  getAllCachedVideos: () => messageSW({ type: 'getAllCachedVideos' }),
  getAutoCachedVideos: () => messageSW({ type: 'getAutoCachedVideos' }),
  getStorageInfo: () => messageSW({ type: 'getStorageInfo' }),
  clearCachedVideos: () => messageSW({ type: 'clearCachedVideos' }),
  clearDownloadedVideos: () => messageSW({ type: 'clearDownloadedVideos' }),

  replayBookmarks: () => messageSW({ type: 'replayBookmarks' }),
};
