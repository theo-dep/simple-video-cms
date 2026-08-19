import { html } from 'htm/preact';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';

const bookmarkedFilter = (v) => v.bookmarked;

export default function Bookmarks() {
  return html`
    <${VideoList} title="Bookmarks" , filterCondition=${bookmarkedFilter} />

    <${Footer} />
  `;
}
