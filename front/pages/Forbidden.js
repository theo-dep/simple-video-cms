import { html } from 'htm/preact';
import { useTitle } from '../hook/useTitle.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/UserNav.js';

export default function Forbidden() {
  useTitle('Forbidden');

  return html`
    <${UserNav} />

    <${InfoContent}>
      <h1>ERROR 403</h1>
      <h3>Sorry! The page you are looking for is forbidden</h3>
      <a href="/" class="back">BACK TO HOME</a>
    <//>
  `;
}
