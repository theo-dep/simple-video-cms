import { html } from 'htm/preact';
import { useTitle } from '../hook/useTitle.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/HeaderNav.js';

export default function NotFound() {
  useTitle('Not Found');

  return html`
    <${UserNav} />

    <${InfoContent}>
      <h1>ERROR 404</h1>
      <h3>Sorry! The page you are looking for can't be found</h3>
      <a href="/" class="back">BACK TO HOME</a>
    <//>
  `;
}
