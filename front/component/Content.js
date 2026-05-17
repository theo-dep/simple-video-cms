import { html } from 'htm/preact';

export function Content({ children, class: cls, ...props }) {
  return html` <div class=${'content' + (cls ? ' ' + cls : '')} ...${props}>${children}</div> `;
}
