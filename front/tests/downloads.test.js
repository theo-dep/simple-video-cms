import { describe, expect, it, vi, beforeEach } from 'vitest';

import { mergeDownloads } from '../pages/Downloads.js';
import { withCacheBadges } from '../component/VideoList.js';
import { refreshCachedVideos, cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';

vi.mock('../store/wb.js', async (importOriginal) => {
  const actual = await importOriginal();
  return {
    ...actual,
    swApi: {
      getAllCachedVideos: vi.fn(),
      getAutoCachedVideos: vi.fn(),
    },
  };
});

import { swApi } from '../store/wb.js';

beforeEach(() => {
  swReady.value = true;
  cache.videos.value = [];
  cache.autoVideos.value = [];
  Object.values(swApi).forEach((mock) => mock.mockReset());
});

describe('refreshCachedVideos', () => {
  it('refreshes both offline and auto-cached videos', async () => {
    const offlineVideos = [{ id: 1, title: 'Downloaded video', playlistUrl: '/api/video/1/playlist', segmentUrls: [] }];
    const autoVideos = [{ id: 2, cachedSegments: 3, totalSegments: 10 }];

    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: offlineVideos } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { videos: autoVideos } });

    await refreshCachedVideos();

    expect(swApi.getAllCachedVideos).toHaveBeenCalledTimes(1);
    expect(swApi.getAutoCachedVideos).toHaveBeenCalledTimes(1);
    expect(cache.videos.value).toEqual(offlineVideos);
    expect(cache.autoVideos.value).toEqual(autoVideos);
  });

  it('keeps empty lists when the service worker does not know the message', async () => {
    swApi.getAllCachedVideos.mockResolvedValueOnce({ type: 'getAllCachedVideosResponse', data: { videos: [] } });
    swApi.getAutoCachedVideos.mockResolvedValueOnce({ type: 'getAutoCachedVideosResponse', data: { success: false, error: 'Unknown message type' } });

    await refreshCachedVideos();

    expect(cache.videos.value).toEqual([]);
    expect(cache.autoVideos.value).toEqual([]);
  });
});

describe('mergeDownloads', () => {
  const allVideos = [
    { id: 1, title: 'Holiday', date: '2024-07-01', location: 'Beach', authors: ['Alice'], tags: ['summer'], bookmarked: true },
    { id: 2, title: 'Birthday', date: '2024-03-12', location: 'Home', authors: [], tags: [] },
  ];

  it('merges offline downloads and auto-cached videos into one list', () => {
    const offlineVideos = [{ id: 1, title: 'Holiday', playlistUrl: '/api/video/1/playlist', segmentUrls: [] }];
    const autoVideos = [
      { id: 2, cachedSegments: 10, totalSegments: 10 },
      { id: 3, cachedSegments: 4, totalSegments: 9 },
    ];

    const merged = mergeDownloads(offlineVideos, autoVideos, allVideos);

    expect(merged).toHaveLength(3);
    expect(merged[0]).toMatchObject({ id: 1, title: 'Holiday' });
    expect(merged[0]).toMatchObject({ date: '2024-07-01', location: 'Beach', tags: ['summer'], bookmarked: true });
    expect(merged[1]).toMatchObject({ id: 2, title: 'Birthday' });
    expect(merged[2]).toMatchObject({ id: 3, title: 'Video #3' });
  });

  it('gives offline downloads priority when a video is in both caches', () => {
    const offlineVideos = [{ id: 1, title: 'Holiday', playlistUrl: '/api/video/1/playlist', segmentUrls: [] }];
    const autoVideos = [{ id: 1, cachedSegments: 2, totalSegments: 9 }];

    const merged = mergeDownloads(offlineVideos, autoVideos, allVideos);

    expect(merged).toHaveLength(1);
    expect(merged[0]).toMatchObject({ id: 1, title: 'Holiday' });
  });

  it('enriches auto-cached videos with the user video metadata', () => {
    const autoVideos = [{ id: 2, cachedSegments: 10, totalSegments: 10 }];

    const merged = mergeDownloads([], autoVideos, allVideos);

    expect(merged[0]).toMatchObject({ title: 'Birthday', date: '2024-03-12', location: 'Home' });
  });
});

describe('withCacheBadges', () => {
  const allVideos = [
    { id: 1, title: 'Holiday' },
    { id: 2, title: 'Birthday' },
    { id: 3, title: 'Wedding' },
  ];

  it('badges downloaded, partially cached and fully cached videos, leaving others untouched', () => {
    const offlineVideos = [{ id: 1, title: 'Holiday' }];
    const autoVideos = [
      { id: 2, cachedSegments: 4, totalSegments: 9 },
      { id: 3, cachedSegments: 9, totalSegments: 9 },
    ];

    const badged = withCacheBadges(allVideos, offlineVideos, autoVideos);

    expect(badged[0]).toMatchObject({ badge: 'downloaded', badgeLabel: 'Downloaded' });
    expect(badged[1]).toMatchObject({ badge: 'partial-cached', badgeLabel: 'Partially cached' });
    expect(badged[2]).toMatchObject({ badge: 'cached', badgeLabel: 'Cached' });
  });

  it('keeps the cached badge when the segment total is unknown', () => {
    const autoVideos = [{ id: 2, cachedSegments: 4, totalSegments: null }, { id: 3 }];

    const badged = withCacheBadges(allVideos, [], autoVideos);

    expect(badged[1]).toMatchObject({ badge: 'cached', badgeLabel: 'Cached' });
    expect(badged[2]).toMatchObject({ badge: 'cached', badgeLabel: 'Cached' });
  });

  it('gives offline downloads priority when a video is in both caches', () => {
    const offlineVideos = [{ id: 1, title: 'Holiday' }];
    const autoVideos = [{ id: 1, cachedSegments: 2, totalSegments: 9 }];

    const badged = withCacheBadges(allVideos, offlineVideos, autoVideos);

    expect(badged[0]).toMatchObject({ badge: 'downloaded', badgeLabel: 'Downloaded' });
  });
});
