import { html } from 'htm/preact';
import { useEffect, useState } from 'preact/hooks';
import { ToggleIconButton } from './ToggleIconButton.js';
import { addVideoToOfflineCache, removeVideoFromOfflineCache, isVideoCached, cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';

export function CacheVideoButton({ id, url, title, location }) {
  const [loading, setLoading] = useState(false);
  const [cached, setCached] = useState(false);
  const [error, setError] = useState(null);

  // Update cached state when global cache changes
  useEffect(() => {
    const checkCached = () => {
      if (swReady.value) {
        setCached(isVideoCached(id));
      }
    };
    checkCached();
    const unsubscribe = cache.videos.subscribe(checkCached);
    return () => unsubscribe();
  }, [id, swReady.value]);

  async function handleCacheToggle(e) {
    e.preventDefault();
    e.stopPropagation();

    setLoading(true);
    setError(null);

    try {
      if (cached) {
        await removeVideoFromOfflineCache(id);
      } else {
        await addVideoToOfflineCache(id, title);
      }
      setCached(isVideoCached(id));
    } catch (error) {
      console.error('Cache toggle error:', error);
      setError(error.message);
      // Revert to actual state
      setCached(isVideoCached(id));
    } finally {
      setLoading(false);
    }
  }

  return html`
    <${ToggleIconButton}
      isActive=${cached}
      onClick=${handleCacheToggle}
      activeIcon="save-fill"
      inactiveIcon="save"
      activeTitle="Remove from download"
      inactiveTitle="Add to download"
      loading=${loading}
      error=${error}
      location=${location}
      className="cache-video-button"
    />
  `;
}
