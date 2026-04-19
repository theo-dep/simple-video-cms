import { html } from 'htm/preact';
import { Content } from './Content.js';

export function InfoContent({ children, class: cls }) {
  return html`
    <${Content}>
      <div class=${'info' + (cls ? ' ' + cls : '')}>${children}</div>
    <//>
  `;
}
