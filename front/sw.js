import { BackgroundSyncPlugin } from 'workbox-background-sync';
import { cacheNames, clientsClaim } from 'workbox-core';
import { precache, getCacheKeyForURL } from 'workbox-precaching';
import { registerRoute } from 'workbox-routing';
import { CacheOnly, StaleWhileRevalidate, CacheFirst, NetworkFirst, NetworkOnly } from 'workbox-strategies';

const VIDEO_ROUTE_PATTERN = [/^\/api\/video\/\d+\/playlist$/, /^\/api\/video\/\d+\/\d+_\d+\.ts$/];
const THUMBNAIL_ROUTE_PATTERN = /^\/api\/thumbnail\/\d+$/;
const BOOKMARK_ROUTE_PATTERN = /^\/api\/bookmark\/\d+$/;
const REFRESH_ROUTE_PATTERN = /^\/api\/refresh$/;
const API_ROUTE_PATTERN = /^\/api\/.*/;

const isVideoRoute = (url) => VIDEO_ROUTE_PATTERN.some((p) => p.test(url.pathname));
const isThumbnailRoute = (url) => THUMBNAIL_ROUTE_PATTERN.test(url.pathname);
const isBookmarkRoute = (url) => BOOKMARK_ROUTE_PATTERN.test(url.pathname);
const isRefreshRoute = (url) => REFRESH_ROUTE_PATTERN.test(url.pathname);
const isAPIRoute = (url) => API_ROUTE_PATTERN.test(url.pathname);

async function log(level, ...message) {
  const clients = await self.clients.matchAll({ includeUncontrolled: true });
  clients.forEach((client) => client.postMessage({ type: 'SW_LOG', level, message }));
}

let videoCachingEnabled = false;
self.addEventListener('message', async (event) => {
  if (event.data === 'disableVideoCaching') {
    await log('log', 'Disable video caching');
    videoCachingEnabled = false;
  } else if (event.data === 'enableVideoCaching') {
    await log('log', 'Enable video caching');
    videoCachingEnabled = true;
  }
});

// shared logging plugin, works for every strategy (CacheOnly included)
function logPlugin(label) {
  return {
    cachedResponseWillBeUsed: async ({ request, cachedResponse }) => {
      if (cachedResponse) await log('log', `Served ${label} from cache:`, request.url);
      return cachedResponse;
    },
    fetchDidSucceed: async ({ request, response }) => {
      if (response) await log('log', `Served ${label} from network:`, request.url);
      return response;
    },
    handlerDidError: async ({ request, error }) => {
      await log('error', `Failed to serve ${label}:`, request.url, error?.message ?? error);
    },
  };
}

self.skipWaiting();
clientsClaim();

precache(self.__WB_MANIFEST);

// assets: CacheOnly, on the precache cache workbox already filled
registerRoute(
  ({ url }) => getCacheKeyForURL(url.href) != null,
  new CacheOnly({
    cacheName: cacheNames.precache,
    plugins: [/*logPlugin('asset')*/],
  })
);

// thumbnails: StaleWhileRevalidate
registerRoute(
  ({ url }) => isThumbnailRoute(url),
  new StaleWhileRevalidate({
    cacheName: 'thumbnails',
    plugins: [logPlugin('thumbnail')],
  })
);

// videos: gated CacheFirst by videoCachingEnabled
class GatedCacheFirst extends CacheFirst {
  async _handle(request, handler) {
    if (!videoCachingEnabled) {
      await log('log', `Served video from network (caching disabled):`, request.url);
      return fetch(request);
    }
    return super._handle(request, handler);
  }
}
registerRoute(
  ({ url }) => isVideoRoute(url),
  new GatedCacheFirst({
    cacheName: 'videos',
    plugins: [logPlugin('video')],
  })
);

// /api/refresh: gates isLogged/videoCachingEnabled client-side, must not stall
registerRoute(
  ({ url }) => isRefreshRoute(url),
  new NetworkFirst({
    cacheName: 'api',
    networkTimeoutSeconds: 3,
    plugins: [
      {
        handlerDidError: logPlugin('refresh').handlerDidError,
      },
    ],
  })
);

// API (non video/thumbnail/refresh): NetworkFirst
registerRoute(
  ({ url }) => isAPIRoute(url) && !isRefreshRoute(url) && !isVideoRoute(url) && !isThumbnailRoute(url),
  new NetworkFirst({
    cacheName: 'api',
    networkTimeoutSeconds: 7,
    plugins: [
      {
        handlerDidError: logPlugin('api').handlerDidError,
      },
    ],
  })
);

// API POST (bookmark): allow background sync
registerRoute(
  ({ url }) => isBookmarkRoute(url),
  new NetworkOnly({
    plugins: [
      new BackgroundSyncPlugin('bookmarkQueue', {
        maxRetentionTime: 24 * 60, // retry for max of 24 Hours (specified in minutes)
      }),
      {
        handlerDidError: logPlugin('bookmark').handlerDidError,
      },
    ],
  }),
  'POST'
);

// index.html: NetworkFirst, never precached (must always fetch latest shell)
registerRoute(
  ({ url }) => !isAPIRoute(url) && getCacheKeyForURL(url.href) == null,
  new NetworkFirst({
    cacheName: 'index',
    networkTimeoutSeconds: 3,
    plugins: [
      {
        handlerDidError: logPlugin('index').handlerDidError,
      },
    ],
  })
);
