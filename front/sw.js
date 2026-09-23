import { Queue } from 'workbox-background-sync';
import { cacheNames, clientsClaim } from 'workbox-core';
import { precache, getCacheKeyForURL } from 'workbox-precaching';
import { registerRoute } from 'workbox-routing';
import { CacheOnly, StaleWhileRevalidate, CacheFirst, NetworkFirst, NetworkOnly } from 'workbox-strategies';
import { formatBytes } from './utils/formatBytes.js';

const VIDEO_PLAYLIST_PATTERN = /^\/api\/video\/(\d+)\/playlist$/;
const VIDEO_SEGMENT_PATTERN = /^\/api\/video\/(\d+)\/\d+_\d+\.ts$/;
const VIDEO_ROUTE_PATTERN = [VIDEO_PLAYLIST_PATTERN, VIDEO_SEGMENT_PATTERN];
const THUMBNAIL_ROUTE_PATTERN = /^\/api\/thumbnail\/\d+$/;
const BOOKMARK_ROUTE_PATTERN = /^\/api\/bookmark\/\d+$/;
const REFRESH_ROUTE_PATTERN = /^\/api\/refresh$/;
const API_ROUTE_PATTERN = /^\/api\/.*/;

const VIDEOS_VERSION = 'v1';
const CACHE_VIDEOS = `videos-${VIDEOS_VERSION}`;
const API_VERSION = 'v1';
const CACHE_API = `api-${API_VERSION}`;
const OFFLINE_CACHE_VERSION = 'v1';
const CACHE_OFFLINE_VIDEOS = `offline-videos-${OFFLINE_CACHE_VERSION}`;
const CACHE_OFFLINE_META = `offline-videos-meta-${OFFLINE_CACHE_VERSION}`;

// Caches kept across versions, all others are deleted on activation
const EXPECTED_CACHES = [cacheNames.precache, CACHE_VIDEOS, CACHE_API, CACHE_OFFLINE_VIDEOS, CACHE_OFFLINE_META, 'thumbnails', 'index'];

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

// In-memory set of video IDs that are cached for offline viewing
const offlineVideoIds = new Set();

self.addEventListener('message', async (event) => {
  if (event.data?.type === 'SKIP_WAITING') {
    await log('log', 'Updated Service Worker skipping waiting');
    self.skipWaiting();
    return;
  }

  if (!event.data?.type || !event.ports?.[0]) return;

  const { type, payload } = event.data;

  try {
    switch (type) {
      case 'enableVideoCaching':
        await log('log', 'Enabling video caching');
        videoCachingEnabled = true;
        break;

      case 'disableVideoCaching':
        await log('log', 'Disabling video caching');
        videoCachingEnabled = false;
        break;

      case 'addVideoToOfflineCache':
        await addVideoToOfflineCache(payload);
        event.ports[0].postMessage({
          type: 'addToOfflineCacheResponse',
          data: { success: true, id: payload.id },
        });
        break;

      case 'removeVideoFromOfflineCache':
        await removeVideoFromOfflineCache(payload);
        event.ports[0].postMessage({
          type: 'removeFromOfflineCacheResponse',
          data: { success: true, id: payload.id },
        });
        break;

      case 'replayBookmarks':
        await replayBookmarkQueue();
        event.ports[0].postMessage({
          type: 'replayBookmarksResponse',
          data: { success: true },
        });
        break;

      case 'getAllCachedVideos':
        {
          const videos = await getAllCachedVideos();
          event.ports[0].postMessage({
            type: 'getAllCachedVideosResponse',
            data: { videos },
          });
        }
        break;

      case 'getAutoCachedVideos':
        {
          const videos = await getAutoCachedVideos();
          event.ports[0].postMessage({
            type: 'getAutoCachedVideosResponse',
            data: { videos },
          });
        }
        break;

      case 'getStorageInfo':
        {
          const storageInfo = await getStorageInfo();
          event.ports[0].postMessage({
            type: 'getStorageInfoResponse',
            data: { storageInfo },
          });
        }
        break;

      case 'clearCachedVideos':
        await clearCachedVideos();
        event.ports[0].postMessage({
          type: 'clearCachedVideosResponse',
          data: { success: true },
        });
        break;

      case 'clearDownloadedVideos':
        await clearDownloadedVideos();
        event.ports[0].postMessage({
          type: 'clearDownloadedVideosResponse',
          data: { success: true },
        });
        break;

      default:
        event.ports[0].postMessage({
          type: `${type}Response`,
          data: { success: false, error: `Unknown message type: ${type}` },
        });
        break;
    }
  } catch (error) {
    // Send error if message has a port
    if (event.ports?.[0]) {
      event.ports[0].postMessage({
        type: `${type}Response`,
        data: { success: false, error: error.message || 'Unknown error' },
      });
    }
    await log('error', `Error in message handler for ${type}:`, error?.message);
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

function extractVideoIdFromUrl(url) {
  const playlistMatch = url.pathname.match(VIDEO_PLAYLIST_PATTERN);
  if (playlistMatch) return parseInt(playlistMatch[1]);
  const segmentMatch = url.pathname.match(VIDEO_SEGMENT_PATTERN);
  if (segmentMatch) return parseInt(segmentMatch[1]);
  return null;
}

// Exact cache usage: navigator.storage.estimate() updates asynchronously
// after cache deletions, so sum the stored entries ourselves
async function getCacheUsage() {
  let usage = 0;
  for (const name of await caches.keys()) {
    const cache = await caches.open(name);
    for (const request of await cache.keys()) {
      const response = await cache.match(request);
      if (!response) continue;
      const length = parseInt(response.headers.get('content-length') || '0');
      usage += length || (await response.arrayBuffer()).byteLength;
    }
  }
  return usage;
}

async function getStorageInfo() {
  const [estimate, usage] = await Promise.all([navigator.storage.estimate(), getCacheUsage()]);
  return {
    quota: estimate.quota,
    usage,
    available: estimate.quota - usage,
    percentageUsed: Math.round((usage / estimate.quota) * 100),
  };
}

// Add a video to the offline cache by downloading its playlist and all segments
async function addVideoToOfflineCache({ id, title }) {
  let session = null;
  try {
    const cache = await caches.open(CACHE_OFFLINE_VIDEOS);
    const metaCache = await caches.open(CACHE_OFFLINE_META);

    // 1. Check if already cached
    const existingMeta = await metaCache.match(`meta-${id}`);
    if (existingMeta) {
      await log('log', `Video ${id} already in offline cache`);
      return { success: true, alreadyCached: true };
    }

    // 2. Check storage space
    const storageInfo = await getStorageInfo();

    // 3. Download playlist first to get segment list
    const playlistUrl = `/api/video/${id}/playlist`;
    const playlistResponse = await fetch(playlistUrl);
    if (!playlistResponse.ok) {
      throw new Error(`Playlist download failed: ${playlistResponse.status}`);
    }
    const playlistContent = await playlistResponse.text();

    // 4. Parse playlist to get segment URLs
    const segmentUrls = [];
    const lines = playlistContent.split('\n');
    for (const line of lines) {
      if (line.trim() && !line.startsWith('#')) {
        segmentUrls.push(`/api/video/${id}/${line.trim()}`);
      }
    }

    // 5. Estimate total size by downloading segments
    let totalSize = playlistContent.length;

    // 6. Enable video session (distinct from the player session)
    const sessionResponse = await fetch(`/api/add-video-session/${id}`, { method: 'POST' });
    session = (await sessionResponse.json().catch(() => null))?.session;
    if (!session) {
      throw new Error('Failed to create video session');
    }
    await fetch(`/api/start-video-session/${id}`, {
      method: 'POST',
      body: new URLSearchParams({ session }),
    });

    for (const segmentUrl of segmentUrls) {
      const response = await fetch(segmentUrl, { method: 'HEAD', headers: { 'X-Video-Session': session } });
      if (!response.ok) {
        throw new Error(`Segment HEAD request failed: ${segmentUrl}`);
      }
      const segmentSize = parseInt(response.headers.get('content-length') || '0');
      totalSize += segmentSize;
    }

    // 7. Check available space (with 10% buffer)
    if (storageInfo.available < totalSize * 1.1) {
      throw new Error('Insufficient storage space');
    }

    // 8. Cache playlist
    await cache.put(playlistUrl, new Response(playlistContent));

    // 9. Cache all segments
    for (const segmentUrl of segmentUrls) {
      const response = await fetch(segmentUrl, { headers: { 'X-Video-Session': session } });
      if (!response.ok) {
        throw new Error(`Segment download failed: ${segmentUrl}`);
      }
      await cache.put(segmentUrl, response.clone());
    }

    // 10. Store metadata
    const meta = {
      id: id,
      title: title,
      playlistUrl,
      segmentUrls,
    };
    await metaCache.put(`meta-${id}`, new Response(JSON.stringify(meta)));

    // 11. Add to in-memory set for fast lookup
    offlineVideoIds.add(id);

    await log('log', `Added video ${id} (${formatBytes(totalSize)}) to offline cache with ${segmentUrls.length} segments`);
    return { success: true };
  } catch (error) {
    await log('error', `Failed to add video ${id} to offline cache:`, error?.message);
    throw error;
  } finally {
    // Clear the server-side video session used for the download.
    // Fire and forget: a failure here must not mask the download outcome.
    if (session) {
      fetch(`/api/clear-video-session/${id}`, {
        method: 'POST',
        body: new URLSearchParams({ session }),
      }).catch(() => {});
    }
  }
}

async function removeVideoFromOfflineCache({ id }) {
  const metaCache = await caches.open(CACHE_OFFLINE_META);
  const metaResponse = await metaCache.match(`meta-${id}`);

  if (metaResponse) {
    const meta = await metaResponse.json();
    const cache = await caches.open(CACHE_OFFLINE_VIDEOS);

    // Remove all segments
    if (meta.segmentUrls) {
      for (const segmentUrl of meta.segmentUrls) {
        await cache.delete(segmentUrl);
      }
    }

    // Remove playlist
    if (meta.playlistUrl) {
      await cache.delete(meta.playlistUrl);
    }

    // Remove metadata
    await metaCache.delete(`meta-${id}`);

    // Remove from in-memory set
    offlineVideoIds.delete(id);

    await log('log', `Removed video ${id} from offline cache`);
  }
}

// Remove the video caches (cached videos)
async function clearCachedVideos() {
  await caches.delete(CACHE_VIDEOS);
  await log('log', `Cleared video cache`);
}

// Remove the offline-video cache (downloaded videos)
async function clearDownloadedVideos() {
  offlineVideoIds.clear();
  await caches.delete(CACHE_OFFLINE_VIDEOS);
  await caches.delete(CACHE_OFFLINE_META);
  await log('log', `Cleared offline-video cache`);
}

// Synchronous check if video is in offline cache
function isVideoCachedSync(id) {
  return offlineVideoIds.has(id);
}

async function getAllCachedVideos() {
  const metaCache = await caches.open(CACHE_OFFLINE_META);
  const cache = await caches.open(CACHE_OFFLINE_VIDEOS);
  const requests = await metaCache.keys();

  const videos = [];
  for (const request of requests) {
    if (request.url.includes('meta-')) {
      const metaResponse = await metaCache.match(request);
      if (metaResponse) {
        const meta = await metaResponse.json();
        if (await cache.match(meta.playlistUrl)) {
          videos.push(meta);
        }
      }
    }
  }
  return videos;
}

// Videos automatically cached by the videos CacheFirst strategy, derived from
// the cached playlist and segment URLs (this cache stores no metadata)
async function getAutoCachedVideos() {
  const cache = await caches.open(CACHE_VIDEOS);
  const requests = await cache.keys();

  const segmentCountById = new Map();
  const playlistUrlById = new Map();

  for (const request of requests) {
    const url = new URL(request.url);
    if (!isVideoRoute(url)) continue;
    const id = extractVideoIdFromUrl(url);
    if (!id) continue;
    if (VIDEO_PLAYLIST_PATTERN.test(url.pathname)) {
      playlistUrlById.set(id, request.url);
    } else {
      segmentCountById.set(id, (segmentCountById.get(id) ?? 0) + 1);
    }
  }

  const videos = [];
  for (const [id, playlistUrl] of playlistUrlById) {
    let totalSegments = null;
    const playlistResponse = await cache.match(playlistUrl);
    if (playlistResponse) {
      const content = await playlistResponse.text();
      totalSegments = content.split('\n').filter((line) => line.trim() && !line.startsWith('#')).length;
    }
    videos.push({
      id,
      cachedSegments: segmentCountById.get(id) ?? 0,
      totalSegments,
    });
  }

  // Segments without a cached playlist (should not happen, kept for robustness)
  for (const [id, cachedSegments] of segmentCountById) {
    if (!playlistUrlById.has(id)) {
      videos.push({ id, cachedSegments, totalSegments: null });
    }
  }

  return videos;
}

// Handle on the queue because Background Sync can silently refuse to fire
// ('sync' registration denied, or never triggered on simulated offline)
const bookmarkQueue = new Queue('bookmarkQueue', {
  maxRetentionTime: 24 * 60, // retry for max of 24 hours (specified in minutes)
});

const bookmarkSyncPlugin = {
  fetchDidFail: async ({ request }) => {
    await bookmarkQueue.pushRequest({ request });
  },
};

let bookmarkReplayInProgress = false;

async function replayBookmarkQueue() {
  if (bookmarkReplayInProgress) return;
  const size = await bookmarkQueue.size();
  if (!size) return;
  bookmarkReplayInProgress = true;
  try {
    await bookmarkQueue.replayRequests();
    await log('log', `Replayed ${size} queued bookmark request(s)`);
  } catch (error) {
    await log('log', `Bookmark queue replay deferred:`, error?.message);
  } finally {
    bookmarkReplayInProgress = false;
  }
}

// The user confirms the update, do not skipWaiting() automatically
clientsClaim();

// Initialize offline video IDs set from existing cache
(async function initializeOfflineCache() {
  try {
    const metaCache = await caches.open(CACHE_OFFLINE_META);
    const requests = await metaCache.keys();

    for (const request of requests) {
      if (request.url.includes('meta-')) {
        const metaResponse = await metaCache.match(request);
        if (metaResponse) {
          const meta = await metaResponse.json();
          if (meta.id) {
            offlineVideoIds.add(meta.id);
          }
        }
      }
    }

    await log('log', `Initialized offline cache with ${offlineVideoIds.size} videos`);
  } catch (error) {
    await log('error', 'Failed to initialize offline cache:', error?.message);
  }
})();

// Drain the queue on SW cold start
void replayBookmarkQueue();

// Delete caches left behind by a previous version (e.g. 'videos-v1' after a bump)
async function cleanupObsoleteCaches() {
  const expected = new Set(EXPECTED_CACHES);
  for (const name of await caches.keys()) {
    if (!expected.has(name)) {
      await caches.delete(name);
      await log('log', `Deleted obsolete cache: ${name}`);
    }
  }
}

self.addEventListener('activate', (event) => {
  event.waitUntil(cleanupObsoleteCaches());
});

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

// Offline videos: CacheOnly for videos marked as offline
// MUST BE BEFORE the normal video route to have priority
registerRoute(
  ({ url }) => {
    // Check if this is a video playlist or segment
    if (!isVideoRoute(url)) {
      return false;
    }

    // Check if this video is in the offline cache
    const id = extractVideoIdFromUrl(url);
    if (!id) return false;

    // Check if video is marked as offline (synchronous)
    return isVideoCachedSync(id);
  },
  new CacheOnly({
    cacheName: CACHE_OFFLINE_VIDEOS,
    plugins: [logPlugin('offline-video')],
  }),
  'GET'
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
    cacheName: CACHE_VIDEOS,
    plugins: [logPlugin('video')],
  })
);

// /api/refresh: gates isLogged/videoCachingEnabled client-side, must not stall
registerRoute(
  ({ url }) => isRefreshRoute(url),
  new NetworkFirst({
    cacheName: CACHE_API,
    networkTimeoutSeconds: 3,
    plugins: [
      {
        handlerDidError: logPlugin('refresh').handlerDidError,
      },
    ],
  })
);

// bookmark POST: NetworkOnly, queued for replay on failure
registerRoute(
  ({ url }) => isBookmarkRoute(url),
  new NetworkOnly({
    plugins: [
      bookmarkSyncPlugin,
      {
        handlerDidError: logPlugin('bookmark').handlerDidError,
      },
    ],
  }),
  'POST'
);

// API: NetworkFirst (last API catch-all)
registerRoute(
  ({ url }) =>
    isAPIRoute(url) && !isRefreshRoute(url) && !isVideoRoute(url) && !isThumbnailRoute(url) && !isBookmarkRoute(url),
  new NetworkFirst({
    cacheName: CACHE_API,
    networkTimeoutSeconds: 7,
    plugins: [
      {
        handlerDidError: logPlugin('api').handlerDidError,
      },
    ],
  })
);

// SPA navigations: NetworkFirst with a bounded timeout, cached shell fallback.
// All routes share one shell, cached under '/'. Offline, the fallback must not
// wait for a hanging fetch to die.
registerRoute(
  ({ request }) => request.mode === 'navigate',
  async ({ request, event }) => {
    const cache = await caches.open('index');

    const timeout = new Promise((_, reject) => {
      setTimeout(() => reject(new Error('shell network timeout')), 3000);
    });

    try {
      const response = await Promise.race([fetch(request), timeout]);
      event.waitUntil(cache.put('/', response.clone()));
      return response;
    } catch (error) {
      await log('error', `Failed to serve shell from network:`, request.url, error?.message ?? error);
      const shell = await cache.match('/');
      if (shell) await log('log', `Served shell from cache for:`, request.url);
      return shell;
    }
  }
);

// Non-precached, non-API files (icons, etc.): NetworkFirst, never precached
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
