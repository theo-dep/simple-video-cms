import { describe, expect, it, vi, beforeEach } from 'vitest';

import { clearCachedVideos, clearDownloadedVideos, addVideoToOfflineCache, removeVideoFromOfflineCache, cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';

vi.mock('../store/wb.js', async (importOriginal) => {
  const actual = await importOriginal();
  return {
    ...actual,
    messageSW: vi.fn(),
  };
});

import { messageSW } from '../store/wb.js';

const storageInfoResponse = {
  type: 'getStorageInfoResponse',
  data: { storageInfo: { quota: 1000, usage: 500, available: 500, percentageUsed: 50 } },
};

beforeEach(() => {
  swReady.value = true;
  cache.storageInfo.value = null;
  cache.videos.value = [];
  cache.autoVideos.value = [];
  messageSW.mockReset();
});

describe('clear cache flows', () => {
  it('refreshes videos and storage info after clearing cached videos', async () => {
    messageSW
      .mockResolvedValueOnce({ type: 'clearCachedVideosResponse', data: { success: true } })
      .mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce(storageInfoResponse);

    await clearCachedVideos();

    expect(messageSW).toHaveBeenNthCalledWith(1, { type: 'clearCachedVideos' });
    expect(messageSW).toHaveBeenNthCalledWith(2, { type: 'getAllCachedVideos' });
    expect(messageSW).toHaveBeenNthCalledWith(3, { type: 'getAutoCachedVideos' });
    expect(messageSW).toHaveBeenNthCalledWith(4, { type: 'getStorageInfo' });
    expect(cache.storageInfo.value).toEqual(storageInfoResponse.data.storageInfo);
  });

  it('throws and does not refresh when the clear fails', async () => {
    messageSW.mockResolvedValueOnce({ type: 'clearDownloadedVideosResponse', data: { success: false, error: 'boom' } });

    await expect(clearDownloadedVideos()).rejects.toThrow('boom');
    expect(cache.storageInfo.value).toBe(null);
  });

  it('refreshes storage info after clearing downloaded videos', async () => {
    messageSW
      .mockResolvedValueOnce({ type: 'clearDownloadedVideosResponse', data: { success: true } })
      .mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce(storageInfoResponse);

    await clearDownloadedVideos();

    expect(messageSW).toHaveBeenNthCalledWith(1, { type: 'clearDownloadedVideos' });
    expect(cache.storageInfo.value).toEqual(storageInfoResponse.data.storageInfo);
  });

  it('refreshes storage info after adding a video to the cache', async () => {
    messageSW
      .mockResolvedValueOnce({ type: 'addToOfflineCacheResponse', data: { success: true, id: 42 } })
      .mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce(storageInfoResponse);

    await addVideoToOfflineCache(42, 'My video');

    expect(messageSW).toHaveBeenNthCalledWith(4, { type: 'getStorageInfo' });
    expect(cache.storageInfo.value).toEqual(storageInfoResponse.data.storageInfo);
  });

  it('refreshes storage info after removing a video from the cache', async () => {
    messageSW
      .mockResolvedValueOnce({ type: 'removeFromOfflineCacheResponse', data: { success: true, id: 42 } })
      .mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: [] } })
      .mockResolvedValueOnce(storageInfoResponse);

    await removeVideoFromOfflineCache(42);

    expect(messageSW).toHaveBeenNthCalledWith(4, { type: 'getStorageInfo' });
    expect(cache.storageInfo.value).toEqual(storageInfoResponse.data.storageInfo);
  });
});
