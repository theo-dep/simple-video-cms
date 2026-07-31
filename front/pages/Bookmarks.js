import { html } from 'htm/preact';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';

export default function Bookmarks() {
  return html`
    <${VideoList} title="Bookmarks" , filterCondition=${(v) => v.bookmarked} />

    <${Footer} />
  `;
}
