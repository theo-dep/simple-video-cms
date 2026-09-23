import { signal, computed } from '@preact/signals';
import { swReady, messageSW } from './wb.js';

// State for cached videos
const cachedVideos = signal([]);
const autoCachedVideos = signal([]);
const storageInfo = signal(null);

// Map for quick access
const cachedVideoIds = computed(() => new Set(cachedVideos.value.map((v) => v.id)));

// Check if a video is cached
export function isVideoCached(videoId) {
  return cachedVideoIds.value.has(videoId);
}

// Enable/disable video caching
export function enableVideoCaching() {
  if (!swReady.value) return;
  messageSW({ type: 'enableVideoCaching' });
}

export function disableVideoCaching() {
  if (!swReady.value) return;
  messageSW({ type: 'disableVideoCaching' });
}

// Refresh the list of cached videos
export async function refreshCachedVideos() {
  if (!swReady.value) return;

  try {
    const [offlineResponse, autoResponse] = await Promise.all([
      messageSW({ type: 'getAllCachedVideos' }),
      messageSW({ type: 'getAutoCachedVideos' }),
    ]);
    cachedVideos.value = offlineResponse?.data?.videos || [];
    autoCachedVideos.value = autoResponse?.data?.videos || [];
  } catch (error) {
    console.error('Error refreshing cached videos:', error);
  }
}

// Refresh storage info
export async function refreshStorageInfo() {
  if (!swReady.value) return;

  try {
    const response = await messageSW({ type: 'getStorageInfo' });
    if (response?.data?.storageInfo) {
      storageInfo.value = response.data.storageInfo;
    }
  } catch (error) {
    console.error('Error refreshing storage info:', error);
  }
}

// Add a video to offline cache
export async function addVideoToOfflineCache(id, title) {
  if (!swReady.value) return;

  try {
    const response = await messageSW({
      type: 'addVideoToOfflineCache',
      payload: { id, title },
    });

    if (response?.data?.success) {
      await refreshCachedVideos();
      await refreshStorageInfo();
    } else {
      throw new Error(response?.data?.error || 'Failed to add video');
    }
  } catch (error) {
    console.error('Error adding video to cache:', error);
    throw error;
  }
}

// Remove a video from offline cache
export async function removeVideoFromOfflineCache(id) {
  if (!swReady.value) return;

  try {
    await messageSW({
      type: 'removeVideoFromOfflineCache',
      payload: { id },
    });
    await refreshCachedVideos();
    await refreshStorageInfo();
  } catch (error) {
    console.error('Error removing video from cache:', error);
    throw error;
  }
}

// Clear the video caches (cached videos)
export async function clearCachedVideos() {
  if (!swReady.value) return;

  try {
    const response = await messageSW({ type: 'clearCachedVideos' });
    if (!response?.data?.success) {
      throw new Error(response?.data?.error || 'Failed to clear video cache');
    }
    await refreshCachedVideos();
    await refreshStorageInfo();
  } catch (error) {
    console.error('Error clearing video cache:', error);
    throw error;
  }
}

// Clear the offline-video cache (downloaded videos)
export async function clearDownloadedVideos() {
  if (!swReady.value) return;

  try {
    const response = await messageSW({ type: 'clearDownloadedVideos' });
    if (!response?.data?.success) {
      throw new Error(response?.data?.error || 'Failed to clear offline-video cache');
    }
    await refreshCachedVideos();
    await refreshStorageInfo();
  } catch (error) {
    console.error('Error clearing offline-video cache:', error);
    throw error;
  }
}

// Exporter le state
export const cache = {
  videos: cachedVideos,
  autoVideos: autoCachedVideos,
  storageInfo,
};
