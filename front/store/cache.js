import { signal, computed } from '@preact/signals';
import { swReady, messageSW } from './wb.js';

// State for cached videos
const cachedVideos = signal([]);
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
    const response = await messageSW({ type: 'getAllCachedVideos' });
    cachedVideos.value = response?.data?.videos || [];
  } catch (error) {
    console.error('Error refreshing cached videos:', error);
  }
}

// Refresh storage info
export async function refreshStorageInfo() {
  if (!swReady.value) return;

  try {
    const response = await messageSW({ type: 'getStorageInfo' });
    storageInfo.value = response?.data?.storageInfo;
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
  } catch (error) {
    console.error('Error removing video from cache:', error);
    throw error;
  }
}

// Format bytes size
export function formatBytes(bytes) {
  if (bytes === 0 || !bytes) return '0 bytes';
  const k = 1024;
  const sizes = ['bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Exporter le state
export const cache = {
  videos: cachedVideos,
  storageInfo,
};
