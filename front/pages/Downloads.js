import { html } from 'htm/preact';
import { useEffect, useMemo } from 'preact/hooks';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';
import { StorageIndicator } from '../component/StorageIndicator.js';
import { cache, refreshCachedVideos } from '../store/cache.js';
import { swReady } from '../store/wb.js';

export default function Downloads() {
  useEffect(() => {
    if (swReady.value) {
      refreshCachedVideos();
    }
  }, [swReady.value]);

  const cachedVideos = useMemo(() => cache.videos.value, [cache.videos.value]);

  return html`
    <${VideoList} title="Downloads" videos=${cachedVideos} searchAside=${html`<${StorageIndicator} />`} />

    <${Footer} />
  `;
}
