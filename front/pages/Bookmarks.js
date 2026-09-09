import { html } from 'htm/preact';
import { useMemo } from 'preact/hooks';
import { user, refreshed } from '../store/auth.js';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';
import { Loader } from '../component/Loader.js';

export default function Bookmarks() {
  const videos = useMemo(() => user.videos.value.filter((v) => v.bookmarked), [user.videos.value]);
  const isLoading = !refreshed.value;

  return html`
    ${isLoading ? html`<${Loader} />` : html`<${VideoList} title="Bookmarks" videos=${videos} />`}

    <${Footer} />
  `;
}
