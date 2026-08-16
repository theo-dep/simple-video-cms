import { html } from 'htm/preact';
import { VideoList } from '../component/VideoList.js';
import { Footer } from '../component/Footer.js';

const allVideosFilter = (_v) => true;

export default function Home() {
  return html`
    <${VideoList} title="Home" , filterCondition=${allVideosFilter} />

    <${Footer} />
  `;
}
