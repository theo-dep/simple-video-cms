import { describe, expect, it, vi, beforeEach } from 'vitest';

import { clearCachedVideos, clearDownloadedVideos, addVideoToOfflineCache, removeVideoFromOfflineCache, cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';

vi.mock('../store/wb.js', async (importOriginal) => {
  const actual = await importOriginal();
  return {
    ...actual,
    swApi: {
      clearCachedVideos: vi.fn(),
      clearDownloadedVideos: vi.fn(),
      addVideoToOfflineCache: vi.fn(),
      removeVideoFromOfflineCache: vi.fn(),
      getAllCachedVideos: vi.fn(),
      getAutoCachedVideos: vi.fn(),
      getStorageInfo: vi.fn(),
    },
  };
});

import { swApi } from '../store/wb.js';

const storageInfo = { quota: 1000, usage: 500, available: 500, percentageUsed: 50 };
const storageInfoResponse = { type: 'getStorageInfoResponse', data: { storageInfo } };

beforeEach(() => {
  swReady.value = true;
  cache.storageInfo.value = null;
  cache.videos.value = [];
  cache.autoVideos.value = [];
  Object.values(swApi).forEach((mock) => mock.mockReset());
});

describe('clear cache flows', () => {
  it('refreshes videos and storage info after clearing cached videos', async () => {
    swApi.clearCachedVideos.mockResolvedValueOnce({ type: 'clearCachedVideosResponse', data: { success: true } });
    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } });
    swApi.getStorageInfo.mockResolvedValueOnce(storageInfoResponse);

    await clearCachedVideos();

    expect(swApi.clearCachedVideos).toHaveBeenCalledTimes(1);
    expect(swApi.getAllCachedVideos).toHaveBeenCalledTimes(1);
    expect(swApi.getAutoCachedVideos).toHaveBeenCalledTimes(1);
    expect(swApi.getStorageInfo).toHaveBeenCalledTimes(1);
    expect(cache.storageInfo.value).toEqual(storageInfo);
  });

  it('throws and does not refresh when the clear fails', async () => {
    swApi.clearDownloadedVideos.mockResolvedValueOnce({ type: 'clearDownloadedVideosResponse', data: { success: false, error: 'boom' } });

    await expect(clearDownloadedVideos()).rejects.toThrow('boom');
    expect(cache.storageInfo.value).toBe(null);
  });

  it('refreshes storage info after clearing downloaded videos', async () => {
    swApi.clearDownloadedVideos.mockResolvedValueOnce({ type: 'clearDownloadedVideosResponse', data: { success: true } });
    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } });
    swApi.getStorageInfo.mockResolvedValueOnce(storageInfoResponse);

    await clearDownloadedVideos();

    expect(swApi.clearDownloadedVideos).toHaveBeenCalledTimes(1);
    expect(cache.storageInfo.value).toEqual(storageInfo);
  });

  it('refreshes storage info after adding a video to the cache', async () => {
    swApi.addVideoToOfflineCache.mockResolvedValueOnce({ type: 'addToOfflineCacheResponse', data: { success: true, id: 42 } });
    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } });
    swApi.getStorageInfo.mockResolvedValueOnce(storageInfoResponse);

    await addVideoToOfflineCache(42, 'My video');

    expect(swApi.addVideoToOfflineCache).toHaveBeenCalledWith(42, 'My video');
    expect(swApi.getStorageInfo).toHaveBeenCalledTimes(1);
    expect(cache.storageInfo.value).toEqual(storageInfo);
  });

  it('refreshes storage info after removing a video from the cache', async () => {
    swApi.removeVideoFromOfflineCache.mockResolvedValueOnce({ type: 'removeFromOfflineCacheResponse', data: { success: true, id: 42 } });
    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } });
    swApi.getStorageInfo.mockResolvedValueOnce(storageInfoResponse);

    await removeVideoFromOfflineCache(42);

    expect(swApi.removeVideoFromOfflineCache).toHaveBeenCalledWith(42);
    expect(swApi.getStorageInfo).toHaveBeenCalledTimes(1);
    expect(cache.storageInfo.value).toEqual(storageInfo);
  });
});
