// Video Service Worker
// Inspired from https://googlechrome.github.io/samples/service-worker/prefetch-video/

// This is one best practice that can be followed in general to keep track of
// multiple caches used by a given service worker, and keep them all versioned.
// It maps a shorthand identifier for a cache to a specific, versioned cache name.

// Note that since global state is discarded in between service worker restarts, these
// variables will be reinitialized each time the service worker handles an event, and you
// should not attempt to change their values inside an event handler. (Treat them as constants.)

// If at any point you want to force pages that use this service worker to start using a fresh
// cache, then increment the CACHE_VERSION value. It will kick off the service worker update
// flow and the old cache(s) will be purged as part of the activate event handler when the
// updated service worker is activated.
const VIDEO_CACHE_VERSION = 1;
const SERVER_ASSETS_CACHE_VERSION = 1;
const RAW_ASSETS_CACHE_VERSION = '__ASSETS_CACHE_VERSION__';
const ASSETS_CACHE_VERSION = RAW_ASSETS_CACHE_VERSION === '__ASSETS_CACHE_VERSION__' ? 'dev' : RAW_ASSETS_CACHE_VERSION;
const CURRENT_CACHES = {
  video: `video-cache-v${VIDEO_CACHE_VERSION}`,
  serverAssets: `server-assets-cache-v${SERVER_ASSETS_CACHE_VERSION}`,
  assets: `assets-cache-v${ASSETS_CACHE_VERSION}`,
};

const VIDEO_ROUTE_PATTERN = [/^\/api\/video\/\d+\/playlist$/, /^\/api\/video\/\d+\/\d+_\d+\.ts$/];
const THUMBNAIL_ROUTE_PATTERN = /^\/api\/thumbnail\/\d+$/;

/* global __ASSETS_MANIFEST__ */
const assets = typeof __ASSETS_MANIFEST__ !== 'undefined' ? __ASSETS_MANIFEST__ : [];
const assetPathSet = new Set(assets.map((assetUrl) => new URL(assetUrl, self.location.origin).pathname));

// Returns true if the given URL matches a dynamic included route
const isVideoRoute = (url) => VIDEO_ROUTE_PATTERN.some((pattern) => pattern.test(url.pathname));
const isThumbnailRoute = (url) => THUMBNAIL_ROUTE_PATTERN.test(url.pathname);

async function log(level, ...message) {
  const clients = await self.clients.matchAll({ includeUncontrolled: true });
  clients.forEach((client) =>
    client.postMessage({
      type: 'SW_LOG',
      level,
      message,
    })
  );
}

let videoCachingEnabled = false;

self.addEventListener('message', (event) => {
  event.waitUntil(log('log', 'Message received:', event.data));

  if (event.data === 'disableVideoCaching') {
    videoCachingEnabled = false;
  } else if (event.data === 'enableVideoCaching') {
    videoCachingEnabled = true;
  }
});

self.addEventListener('install', (event) => {
  event.waitUntil(
    (async () => {
      await log('log', 'Install: caching assets and skip waiting state');

      if (assets && assets.length > 0) {
        const cache = await caches.open(CURRENT_CACHES.assets);
        // add asset per asset to not break the Service Worker in case of one asset missing
        const results = await Promise.allSettled(assets.map((asset) => cache.add(asset)));
        await Promise.all(
          results.map((result, i) =>
            result.status === 'fulfilled'
              ? log('log', 'Asset precached:', assets[i])
              : log('error', 'Failed to precache asset:', assets[i], result.reason)
          )
        );
      }
      await self.skipWaiting();
    })()
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    (async () => {
      await log('log', 'Activate: purging outdated caches');

      // Delete all caches that aren't named in CURRENT_CACHES.
      // While there is only one cache in this example, the same logic will handle the case where
      // there are multiple versioned caches.
      const expectedCacheNames = Object.values(CURRENT_CACHES);

      const cacheNames = await caches.keys();

      await Promise.all(
        cacheNames
          .filter((name) => !expectedCacheNames.includes(name))
          .map(async (name) => {
            // If this cache name isn't present in the array of "expected" cache names, then delete it.
            await log('log', 'Deleting outdated cache:', name);
            return caches.delete(name);
          })
      );

      await self.clients.claim(); // Take control immediately
    })()
  );
});

function fromCatchOrFetch(cacheName, label, throwOnNetworkError = false) {
  return async (event, request, url) => {
    const cache = await caches.open(cacheName);

    // caches.match() will look for a cache entry in all of the caches available to the service worker.
    // It's an alternative to first opening a specific named cache and then matching on that.
    const cached = await cache.match(request);
    if (cached) {
      event.waitUntil(log('log', `Serving ${label} from cache:`, url.href));
      return cached;
    }

    // event.request will always have the proper mode set ('cors, 'no-cors', etc.) so we don't
    // have to hardcode 'no-cors' like we do when fetch()ing in the install handler.
    const response = await fetch(request);

    if (response.ok) {
      event.waitUntil(log('log', `Caching ${label}:`, url.href));
      event.waitUntil(cache.put(request, response.clone()));
    } else if (throwOnNetworkError) {
      throw new Error(`Server returned ${response.status}`);
    }
    return response;
  };
}

self.addEventListener('fetch', (event) => {
  const { request } = event;
  const url = new URL(request.url);

  if (assetPathSet.has(url.pathname)) {
    event.respondWith(fromCatchOrFetch(CURRENT_CACHES.assets, 'asset')(event, request, url));
    return;
  }

  if (isThumbnailRoute(url)) {
    event.respondWith(fromCatchOrFetch(CURRENT_CACHES.serverAssets, 'thumbnail')(event, request, url));
    return;
  }

  if (!isVideoRoute(url)) {
    return; // Let the browser handle it natively
  }

  if (!videoCachingEnabled) {
    //event.waitUntil(log('log', 'Caching disabled, fetch:', url.href));
    return;
  }

  event.respondWith(
    (async () => {
      try {
        return await fromCatchOrFetch(CURRENT_CACHES.video, 'video', true)(event, request, url);
      } catch (err) {
        // This catch() will handle exceptions thrown from the fetch() operation.
        // Note that a HTTP error response (e.g. 404) will NOT trigger an exception.
        // It will return a normal response object that has the appropriate error code set.
        event.waitUntil(log('error', 'Failed to serve:', err));
        throw err;
      }
    })()
  );
});
