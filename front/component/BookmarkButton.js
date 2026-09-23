import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { api } from '../api.js';
import { user } from '../store/auth.js';
import { ToggleIconButton } from './ToggleIconButton.js';

export function BookmarkButton({ videoId, isBookmarked, location }) {
  const video = user.videos.value.find((v) => v.id === Number(videoId));
  const [bookmarked, setBookmarked] = useState(isBookmarked);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  async function handleToggle(e) {
    e.preventDefault();
    e.stopPropagation();

    setLoading(true);
    setError(null);

    const newBookmarked = !bookmarked;
    video.bookmarked = newBookmarked;
    setBookmarked(newBookmarked); // optimistic, before await

    try {
      await api.bookmark(videoId, newBookmarked);
    } catch (err) {
      if (navigator.onLine) {
        // real failure, revert
        console.error('Bookmark set failed:', err);
        video.bookmarked = bookmarked;
        setBookmarked(bookmarked);
        setError(err.message);
      }
      // offline: request is queued by SW background sync, keep optimistic state
    } finally {
      setLoading(false);
    }
  }

  return html`
    <${ToggleIconButton}
      isActive=${bookmarked}
      onClick=${handleToggle}
      activeIcon="bookmark-star-fill"
      inactiveIcon="bookmark-star"
      activeTitle="Remove from bookmarks"
      inactiveTitle="Add to bookmarks"
      loading=${loading}
      error=${error}
      location=${location}
    />
  `;
}
