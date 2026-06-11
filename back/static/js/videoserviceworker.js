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
  prefetch: `prefetch-cache-v${CACHE_VERSION}`,
  video: `video-cache-v${CACHE_VERSION}`,
};

const ASSETS_TO_PREFETCH = [
  '/static/css/third-party/pure-min.css',
  '/static/css/third-party/grids-responsive-min.css',
  '/static/css/third-party/videojs-min.css',
  '/static/css/third-party/videojs-mobile-ui-min.css',
  '/static/css/third-party/videojs-yt-style-min.css',
  '/static/css/styles.css',
  '/static/css/video.css',
  '/static/js/third-party/videojs-min.js',
  '/static/js/third-party/videojs-mobile-ui-min.js',
  '/static/js/third-party/videojs-yt-style-min.js',
];

const VIDEO_ROUTE_PATTERN = /^\/video\/\d+$/;
const EXCLUDED_ROUTE_PATTERN = [
  /^\/add-video-session\/\d+$/,
  /^\/start-video-session\/\d+$/,
  /^\/reset-video-session\/\d+$/,
  /^\/increment-video-views\/\d+$/
];

// Returns true if the given URL matches a dynamic video route (/video/<id>).
const isVideoRoute = (url) => VIDEO_ROUTE_PATTERN.test(new URL(url).pathname);

// Returns true if the given URL matches a dynamic excluded route
const isExcludedRoute = (url) => EXCLUDED_ROUTE_PATTERN.some(pattern => pattern.test(new URL(url).pathname));

self.addEventListener('install', (event) => {
  console.log('[SW] Install: prefetching static assets');
  self.skipWaiting();

  event.waitUntil(
    caches.open(CURRENT_CACHES.prefetch)
      .then((cache) => cache.addAll(ASSETS_TO_PREFETCH))
      .then(() => console.log('[SW] Static assets prefetched successfully'))
      .catch((err) => console.error('[SW] Prefetch failed:', err))
  );
});

self.addEventListener('activate', (event) => {
  console.log('[SW] Activate: purging outdated caches');

  // Delete all caches that aren't named in CURRENT_CACHES.
  // While there is only one cache in this example, the same logic will handle the case where
  // there are multiple versioned caches.
  const expectedCacheNames = Object.values(CURRENT_CACHES);

  event.waitUntil(
    caches.keys()
      .then((cacheNames) =>
        Promise.all(
          cacheNames
            .filter((name) => !expectedCacheNames.includes(name))
            .map((name) => {
              // If this cache name isn't present in the array of "expected" cache names, then delete it.
              console.log('[SW] Deleting outdated cache:', name);
              return caches.delete(name);
            })
        )
      )
      .then(() => self.clients.claim()) // Take control immediately
  );
});

self.addEventListener('fetch', (event) => {
  const { request } = event;
  const isVideo = isVideoRoute(request.url);

  if (isExcludedRoute(request.url)) {
    return; // Let the browser handle it natively
  }

  const currentCache = isVideo ? CURRENT_CACHES.video : CURRENT_CACHES.prefetch;

  event.respondWith(
    caches.open(currentCache)
      // caches.match() will look for a cache entry in all of the caches available to the service worker.
      // It's an alternative to first opening a specific named cache and then matching on that.
      .then((cache) => cache.match(request.url))
      .then((cached) => {
        if (cached) {
          //console.log('[SW] Serving from cache:', request.url);
          return cached;
        }
        // event.request will always have the proper mode set ('cors, 'no-cors', etc.) so we don't
        // have to hardcode 'no-cors' like we do when fetch()ing in the install handler.
        return fetch(request).then((response) => {
          if (!response.ok) {
            throw new Error(`Server returned ${response.status}`);
          }
          return caches.open(currentCache).then((cache) => {
            console.log('[SW] Caching: ', request.url);
            cache.put(request.url, response.clone());
            return response;
          });
        });
      })
      .catch((err) => {
        // This catch() will handle exceptions thrown from the fetch() operation.
        // Note that a HTTP error response (e.g. 404) will NOT trigger an exception.
        // It will return a normal response object that has the appropriate error code set.
        console.error('[SW] Failed to serve:', err);
        throw err;
      })
  );
});
