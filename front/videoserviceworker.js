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
const CACHE_VERSION = 1;
const CURRENT_CACHES = {
  video: `video-cache-v${CACHE_VERSION}`,
};

const INCLUDED_ROUTE_PATTERN = [/^\/api\/video\/\d+\/playlist$/, /^\/api\/video\/\d+\/\d+_\d+\.ts$/];

// Returns true if the given URL matches a dynamic included route
const isIncludedRoute = (url) => INCLUDED_ROUTE_PATTERN.some((pattern) => pattern.test(new URL(url).pathname));

async function log(level, ...message) {
  const clients = await self.clients.matchAll();
  clients.forEach((client) =>
    client.postMessage({
      type: 'SW_LOG',
      level,
      message,
    })
  );
}

self.addEventListener('activate', (event) => {
  log('log', 'Activate: purging outdated caches');

  // Delete all caches that aren't named in CURRENT_CACHES.
  // While there is only one cache in this example, the same logic will handle the case where
  // there are multiple versioned caches.
  const expectedCacheNames = Object.values(CURRENT_CACHES);

  event.waitUntil(
    caches
      .keys()
      .then((cacheNames) =>
        Promise.all(
          cacheNames
            .filter((name) => !expectedCacheNames.includes(name))
            .map((name) => {
              // If this cache name isn't present in the array of "expected" cache names, then delete it.
              log('log', 'Deleting outdated cache:', name);
              return caches.delete(name);
            })
        )
      )
      .then(() => self.clients.claim()) // Take control immediately
  );
});

self.addEventListener('fetch', (event) => {
  const { request } = event;

  if (!isIncludedRoute(request.url)) {
    return; // Let the browser handle it natively
  }

  const currentCache = CURRENT_CACHES.video;

  event.respondWith(
    caches
      .open(currentCache)
      // caches.match() will look for a cache entry in all of the caches available to the service worker.
      // It's an alternative to first opening a specific named cache and then matching on that.
      .then((cache) => cache.match(request.url))
      .then((cached) => {
        if (cached) {
          log('log', 'Serving from cache:', request.url);
          return cached;
        }
        // event.request will always have the proper mode set ('cors, 'no-cors', etc.) so we don't
        // have to hardcode 'no-cors' like we do when fetch()ing in the install handler.
        return fetch(request).then((response) => {
          if (!response.ok) {
            throw new Error(`Server returned ${response.status}`);
          }
          return caches.open(currentCache).then((cache) => {
            log('log', 'Caching: ', request.url);
            cache.put(request.url, response.clone());
            return response;
          });
        });
      })
      .catch((err) => {
        // This catch() will handle exceptions thrown from the fetch() operation.
        // Note that a HTTP error response (e.g. 404) will NOT trigger an exception.
        // It will return a normal response object that has the appropriate error code set.
        log('error', 'Failed to serve:', err);
        throw err;
      })
  );
});
