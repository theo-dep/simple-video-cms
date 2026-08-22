import { cacheNames, clientsClaim } from 'workbox-core';
import { precache, getCacheKeyForURL } from 'workbox-precaching';
import { registerRoute } from 'workbox-routing';
import { CacheOnly, StaleWhileRevalidate, CacheFirst, NetworkFirst } from 'workbox-strategies';

const VIDEO_ROUTE_PATTERN = [/^\/api\/video\/\d+\/playlist$/, /^\/api\/video\/\d+\/\d+_\d+\.ts$/];
const THUMBNAIL_ROUTE_PATTERN = /^\/api\/thumbnail\/\d+$/;
const REFRESH_ROUTE_PATTERN = /^\/api\/refresh$/;
const API_ROUTE_PATTERN = /^\/api\/.*/;

const isVideoRoute = (url) => VIDEO_ROUTE_PATTERN.some((p) => p.test(url.pathname));
const isThumbnailRoute = (url) => THUMBNAIL_ROUTE_PATTERN.test(url.pathname);
const isRefreshRoute = (url) => REFRESH_ROUTE_PATTERN.test(url.pathname);
const isAPIRoute = (url) => API_ROUTE_PATTERN.test(url.pathname);

let videoCachingEnabled = false;
self.addEventListener('message', (event) => {
  if (event.data === 'disableVideoCaching') videoCachingEnabled = false;
  else if (event.data === 'enableVideoCaching') videoCachingEnabled = true;
});

async function log(level, ...message) {
  const clients = await self.clients.matchAll({ includeUncontrolled: true });
  clients.forEach((client) => client.postMessage({ type: 'SW_LOG', level, message }));
}

// shared logging plugin, works for every strategy (CacheOnly included)
function logPlugin(label) {
  return {
    handlerDidRespond: async ({ request, response }) => {
      if (response) await log('log', `Served ${label} from cache/network:`, request.url);
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
    plugins: [logPlugin('asset')],
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
    if (!videoCachingEnabled) return fetch(request);
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
registerRoute(({ url }) => isRefreshRoute(url), new NetworkFirst({ cacheName: 'api', networkTimeoutSeconds: 3, plugins: [logPlugin('refresh')] }));

// API (non video/thumbnail/refresh): NetworkFirst
registerRoute(
  ({ url }) => isAPIRoute(url) && !isRefreshRoute(url) && !isVideoRoute(url) && !isThumbnailRoute(url),
  new NetworkFirst({ cacheName: 'api', networkTimeoutSeconds: 7, plugins: [logPlugin('api')] })
);

// index.html: NetworkFirst, never precached (must always fetch latest shell)
registerRoute(
  ({ url }) => !isAPIRoute(url) && getCacheKeyForURL(url.href) == null,
  new NetworkFirst({ cacheName: 'index', networkTimeoutSeconds: 3, plugins: [logPlugin('index')] })
);
