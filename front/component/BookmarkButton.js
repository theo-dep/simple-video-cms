import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { api } from '../api.js';
import { user } from '../store/auth.js';
import { Icon } from './Icon.js';

export function BookmarkButton({ videoId, isBookmarked, location }) {
  const video = user.videos.value.find((v) => v.id === Number(videoId));
  const [bookmarked, setBookmarked] = useState(isBookmarked);

  async function addToBookmarks(e) {
    e.preventDefault();
    e.stopPropagation();
    const newBookmarked = !bookmarked;
    await api.bookmark(videoId, newBookmarked);
    video.bookmarked = newBookmarked;
    setBookmarked(newBookmarked);
  }

  const icon = bookmarked
    ? html`<${Icon} name="bookmark-star" fill />`
    : html`<${Icon} name="bookmark-star" />`;

  return html`
    <div
      class="bookmark-button ${bookmarked ? 'is-bookmarked' : ''}"
      location=${location}
      onClick=${addToBookmarks}
      title=${bookmarked ? 'Remove from bookmarks' : 'Add to bookmarks'}
    >
      ${icon}
    </div>
  `;
}
