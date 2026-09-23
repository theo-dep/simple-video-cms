import { html } from 'htm/preact';
import { useMemo } from 'preact/hooks';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';
import { StorageIndicator } from '../component/StorageIndicator.js';
import { ClearCacheButtons } from '../component/ClearCacheButtons.js';
import { cache } from '../store/cache.js';
import { user } from '../store/auth.js';

// Merge offline downloads and automatically cached videos into a single list
export function mergeDownloads(offlineVideos, autoVideos, allVideos) {
  const metaById = new Map(allVideos.map((v) => [v.id, v]));
  const merged = new Map();

  for (const video of offlineVideos) {
    merged.set(video.id, { ...metaById.get(video.id), ...video });
  }

  for (const video of autoVideos) {
    if (merged.has(video.id)) continue;
    const meta = metaById.get(video.id);
    merged.set(video.id, {
      ...meta,
      id: video.id,
      title: meta?.title ?? `Video #${video.id}`,
    });
  }

  return [...merged.values()];
}

export default function Downloads() {
  const downloads = useMemo(
    () => mergeDownloads(cache.videos.value, cache.autoVideos.value, user.videos.value),
    [cache.videos.value, cache.autoVideos.value, user.videos.value]
  );

  return html`
    <${VideoList} title="Downloads" videos=${downloads} searchAside=${html`<${StorageIndicator} /><${ClearCacheButtons} />`} />

    <${Footer} />
  `;
}
