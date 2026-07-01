import { html } from 'htm/preact';
import { useEffect, useRef } from 'preact/hooks';

export function Loader() {
  const loaderRef = useRef(null);

  useEffect(() => {
    const loaderTemplateHtml = globalThis.document?.getElementById('loader-template')?.innerHTML?.trim() ?? '';
    loaderRef.current.innerHTML = loaderTemplateHtml;
  }, []);

  return html`<div ref=${loaderRef} role="status" aria-label="Loading"></div>`;
}
