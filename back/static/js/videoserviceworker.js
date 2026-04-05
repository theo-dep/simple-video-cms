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
const CACHE_VERSION = 2;
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
const INCREMENT_VIDEO_VIEWS_ROUTE_PATTERN = /^\/increment_video_views\/\d+$/;

// In-progress video downloads, keyed by URL, to avoid duplicate fetches.
const videoDownloadsInProgress = {};

// Returns true if the given URL matches a dynamic video route (/video/<id>).
const isVideoRoute = (url) => VIDEO_ROUTE_PATTERN.test(new URL(url).pathname);

// Returns true if the given URL matches a dynamic increment video views route (/video/<id>).
const isIncrementVideoViewsRoute = (url) => INCREMENT_VIDEO_VIEWS_ROUTE_PATTERN.test(new URL(url).pathname);

// Parses the starting byte position from a Range header value.
// Supports the "bytes=<start>-" format emitted by most browsers.
const parseRangeStart = (rangeHeader) => {
  const match = /^bytes=(\d+)-$/i.exec(rangeHeader);
  if (!match) {
    throw new Error(`Unsupported Range header format: ${rangeHeader}`);
  }
  return Number(match[1]);
};

// Builds a 206 Partial Content response from an ArrayBuffer and a start position.
const buildPartialResponse = (buffer, start) => new Response(
  buffer.slice(start), {
    status: 206,
    statusText: 'Partial Content',
    headers: {
      'Content-Type': 'video/mp4',
      'Content-Range': `bytes ${start}-${buffer.byteLength - 1}/${buffer.byteLength}`,
    },
  }
);

// Fetches a video in full, stores it in the video cache, and resolves with its ArrayBuffer.
// Deduplicates concurrent requests for the same URL.
const fetchAndCacheVideo = (request) => {
  const { url } = request;

  if (videoDownloadsInProgress[url]) {
    console.log('[SW] Reusing in-progress download for:', url);
    return videoDownloadsInProgress[url];
  }

  console.log('[SW] Starting full video download for:', url);

  // Create a new request without the Range header to always get a full 200 response
  const fullRequest = new Request(url, {
    headers: (() => {
      const headers = new Headers(request.headers);
      headers.delete('range');
      return headers;
    })(),
    credentials: request.credentials,
    cache: request.cache,
    mode: request.mode,
    redirect: request.redirect,
  });

  const promise = fetch(fullRequest)
    .then((response) => {
      if (response.status !== 200) {
        throw new Error(`Server returned ${response.status} for ${url}`);
      }
      return caches.open(CURRENT_CACHES.video).then((cache) => {
        cache.put(url, response.clone());
        console.log('[SW] Video successfully cached:', url);
        return response.arrayBuffer();
      });
    })
    .finally(() => {
      delete videoDownloadsInProgress[url];
    });

  videoDownloadsInProgress[url] = promise;
  return promise;
};

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
    caches.keys().then((cacheNames) =>
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
  );
});

self.addEventListener('fetch', (event) => {
  const { request } = event;
  const rangeHeader = request.headers.get('range');
  const isRange = !!rangeHeader;
  const isVideo = isVideoRoute(request.url);

  if (isIncrementVideoViewsRoute(request.url)) {
    return; // Let the browser handle it natively
  }

  // Range request on a video route => serve from cache or fetch full video
  if (isRange && isVideo) {
    let pos;
    try {
      pos = parseRangeStart(rangeHeader);
    } catch (err) {
      console.error('[SW] Invalid Range header:', err);
      return; // Let the browser handle it natively
    }

    console.log(`[SW] Video range request: ${request.url} (pos: ${pos})`);

    event.respondWith(
      caches.open(CURRENT_CACHES.video)
        .then((cache) => cache.match(request.url))
        .then((cached) => {
          if (cached) {
            console.log('[SW] Serving video range from cache:', request.url);
            return cached.arrayBuffer();
          }
          console.log('[SW] Cache miss: fetching full video:', request.url);
          return fetchAndCacheVideo(request);
        })
        .then((buffer) => buildPartialResponse(buffer, pos))
        .catch((err) => {
          console.error('[SW] Failed to serve video range, falling back to network:', err);
          return fetch(request);
        })
    );

  // Non-range request on a video route => cache-first, then network
  } else if (isVideo) {
    console.log('[SW] Non-range video request:', request.url);

    event.respondWith(
      caches.open(CURRENT_CACHES.video)
        // caches.match() will look for a cache entry in all of the caches available to the service worker.
        // It's an alternative to first opening a specific named cache and then matching on that.
        .then((cache) => cache.match(request.url))
        .then((cached) => {
          if (cached) {
            console.log('[SW] Serving video from cache:', request.url);
            return cached;
          }
          // event.request will always have the proper mode set ('cors, 'no-cors', etc.) so we don't
          // have to hardcode 'no-cors' like we do when fetch()ing in the install handler.
          return fetch(request).then((response) => {
            if (!response.ok) {
              throw new Error(`Server returned ${response.status}`);
            }
            return caches.open(CURRENT_CACHES.video).then((cache) => {
              cache.put(request.url, response.clone());
              return response;
            });
          });
        })
        .catch((err) => {
          // This catch() will handle exceptions thrown from the fetch() operation.
          // Note that a HTTP error response (e.g. 404) will NOT trigger an exception.
          // It will return a normal response object that has the appropriate error code set.
          console.error('[SW] Failed to serve video:', err);
          throw err;
        })
    );

  // Range request on a prefetched static asset
  } else if (isRange) {
    let pos;
    try {
      pos = parseRangeStart(rangeHeader);
    } catch (err) {
      console.error('[SW] Invalid Range header:', err);
      return;
    }

    event.respondWith(
      caches.open(CURRENT_CACHES.prefetch)
        .then((cache) => cache.match(request.url))
        .then((cached) => cached
          ? cached.arrayBuffer()
          : fetch(request).then((r) => r.arrayBuffer())
        )
        .then((buffer) => buildPartialResponse(buffer, pos))
    );

  // Standard request => cache-first, then network
  } else {
    event.respondWith(
      caches.open(CURRENT_CACHES.prefetch)
        .then((cache) => cache.match(request.url)
          .then((cached) => {
            if (cached) {
              console.log('[SW] Serving prefetch asset from cache:', request.url);
              return cached;
            }
            console.log('[SW] Cache miss: fetching and caching prefetch asset:', request.url);
            return fetch(request).then((response) => {
              if (!response.ok) throw new Error(`Server returned ${response.status}`);
              cache.put(request.url, response.clone());
              return response;
            }).catch((err) => {
              console.error('[SW] Network fetch failed:', err);
              throw err;
            });
          })
        )
    );
  }
});
